#ifndef SPLITTUNNELINGEXTRASCONTROLLER_H
#define SPLITTUNNELINGEXTRASCONTROLLER_H

#include <QObject>
#include <QString>

#include "core/repositories/secureAppSettingsRepository.h"

class QNetworkAccessManager;
class QNetworkReply;

// Управление флажками «Исключить российский трафик» / «Исключить локальный трафик»
// и обновлением списка российских подсетей.
class SplitTunnelingExtrasController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool excludeRussia READ excludeRussia WRITE setExcludeRussia NOTIFY excludeRussiaChanged)
    Q_PROPERTY(bool excludeLocal READ excludeLocal WRITE setExcludeLocal NOTIFY excludeLocalChanged)
    Q_PROPERTY(bool autoUpdate READ autoUpdate WRITE setAutoUpdate NOTIFY autoUpdateChanged)
    Q_PROPERTY(bool updating READ updating NOTIFY updatingChanged)
    Q_PROPERTY(int subnetsCount READ subnetsCount NOTIFY subnetsChanged)
    // Дата последнего скачивания списка; пустая строка — используется встроенный снимок
    Q_PROPERTY(QString updatedAtText READ updatedAtText NOTIFY subnetsChanged)

public:
    explicit SplitTunnelingExtrasController(SecureAppSettingsRepository *appSettingsRepository, QObject *parent = nullptr);

    bool excludeRussia() const;
    bool excludeLocal() const;
    bool autoUpdate() const;
    bool updating() const { return m_updating; }
    int subnetsCount() const;
    QString updatedAtText() const;

public slots:
    void setExcludeRussia(bool enabled);
    void setExcludeLocal(bool enabled);
    void setAutoUpdate(bool enabled);

    // Скачать свежий список российских подсетей
    void updateSubnets();

signals:
    void excludeRussiaChanged();
    void excludeLocalChanged();
    void autoUpdateChanged();
    void updatingChanged();
    void subnetsChanged();
    // ok — список обновлён; count — число подсетей в новом списке
    void updateFinished(bool ok, int count);

private:
    void maybeAutoUpdate();
    void setUpdating(bool updating);

    SecureAppSettingsRepository *m_appSettingsRepository;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    bool m_updating = false;
};

#endif // SPLITTUNNELINGEXTRASCONTROLLER_H
