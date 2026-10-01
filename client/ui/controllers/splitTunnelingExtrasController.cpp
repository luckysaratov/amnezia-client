#include "splitTunnelingExtrasController.h"

#include <QDateTime>
#include <QDebug>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include "core/utils/splitTunnelingExtras.h"

namespace
{
    // Список стран от ipverse (данные RIR), обновляется ежедневно
    const char kRuSubnetsUrl[] =
            "https://raw.githubusercontent.com/ipverse/country-ip-blocks/master/country/ru/aggregated.json";
    constexpr int kAutoUpdateDays = 7;
    constexpr int kAutoUpdateDelayMs = 15000;
    constexpr int kDownloadTimeoutMs = 30000;
    constexpr qint64 kMaxDownloadBytes = 8 * 1024 * 1024;
}

SplitTunnelingExtrasController::SplitTunnelingExtrasController(SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent), m_appSettingsRepository(appSettingsRepository)
{
    m_network = new QNetworkAccessManager(this);
    // Автообновление списка вскоре после запуска, чтобы не тормозить старт
    QTimer::singleShot(kAutoUpdateDelayMs, this, &SplitTunnelingExtrasController::maybeAutoUpdate);
}

bool SplitTunnelingExtrasController::excludeRussia() const
{
    return m_appSettingsRepository->isExcludeRussianTraffic();
}

bool SplitTunnelingExtrasController::excludeLocal() const
{
    return m_appSettingsRepository->isExcludeLocalTraffic();
}

bool SplitTunnelingExtrasController::autoUpdate() const
{
    return m_appSettingsRepository->isRussianSubnetsAutoUpdateEnabled();
}

int SplitTunnelingExtrasController::subnetsCount() const
{
    return SplitTunnelingExtras::russianSubnetsCount();
}

QString SplitTunnelingExtrasController::updatedAtText() const
{
    const QDateTime at = SplitTunnelingExtras::russianSubnetsUpdatedAt();
    if (!at.isValid()) {
        return QString();
    }
    return QLocale().toString(at, QLocale::ShortFormat);
}

void SplitTunnelingExtrasController::setExcludeRussia(bool enabled)
{
    if (excludeRussia() == enabled) {
        return;
    }
    m_appSettingsRepository->setExcludeRussianTraffic(enabled);
    emit excludeRussiaChanged();
}

void SplitTunnelingExtrasController::setExcludeLocal(bool enabled)
{
    if (excludeLocal() == enabled) {
        return;
    }
    m_appSettingsRepository->setExcludeLocalTraffic(enabled);
    emit excludeLocalChanged();
}

void SplitTunnelingExtrasController::setAutoUpdate(bool enabled)
{
    if (autoUpdate() == enabled) {
        return;
    }
    m_appSettingsRepository->setRussianSubnetsAutoUpdateEnabled(enabled);
    emit autoUpdateChanged();
}

void SplitTunnelingExtrasController::setUpdating(bool updating)
{
    if (m_updating == updating) {
        return;
    }
    m_updating = updating;
    emit updatingChanged();
}

void SplitTunnelingExtrasController::maybeAutoUpdate()
{
    if (!autoUpdate()) {
        return;
    }
    const QDateTime at = SplitTunnelingExtras::russianSubnetsUpdatedAt();
    if (at.isValid() && at.daysTo(QDateTime::currentDateTime()) < kAutoUpdateDays) {
        return;
    }
    updateSubnets();
}

void SplitTunnelingExtrasController::updateSubnets()
{
    if (m_updating) {
        return;
    }
    setUpdating(true);

    QNetworkRequest request{ QUrl(QString::fromLatin1(kRuSubnetsUrl)) };
    request.setTransferTimeout(kDownloadTimeoutMs);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->deleteLater();

        bool ok = false;
        int count = 0;
        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray data = reply->read(kMaxDownloadBytes);
            QStringList subnets;
            if (SplitTunnelingExtras::parseRussianSubnetsJson(data, subnets)
                && SplitTunnelingExtras::saveRussianSubnets(subnets)) {
                ok = true;
                count = subnets.size();
                qInfo() << "[SplitTunneling] список российских подсетей обновлён:" << count;
            } else {
                qWarning() << "[SplitTunneling] не удалось разобрать или сохранить список российских подсетей";
            }
        } else {
            qWarning() << "[SplitTunneling] ошибка загрузки списка российских подсетей:" << reply->errorString();
        }

        setUpdating(false);
        if (ok) {
            emit subnetsChanged();
        }
        emit updateFinished(ok, count);
    });
}
