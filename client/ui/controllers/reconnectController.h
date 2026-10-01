#ifndef RECONNECTCONTROLLER_H
#define RECONNECTCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QPointer>
#include <QList>
#include <QPair>

#include "core/controllers/connectionController.h"
#include "core/controllers/serversController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/protocols/vpnProtocol.h"

class QProcess;

// Periodically pings a user-defined list of hosts while the VPN is connected and, when they are
// unreachable, restarts the connection. This is the core feature of the "reconnect" fork.
class ReconnectController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int intervalSeconds READ intervalSeconds WRITE setIntervalSeconds NOTIFY intervalSecondsChanged)
    Q_PROPERTY(int failThreshold READ failThreshold WRITE setFailThreshold NOTIFY failThresholdChanged)
    Q_PROPERTY(int failMode READ failMode WRITE setFailMode NOTIFY failModeChanged)
    // Резервный сервер: при обрыве переключаемся на него, а текущий становится резервным
    Q_PROPERTY(bool failoverEnabled READ isFailoverEnabled WRITE setFailoverEnabled NOTIFY failoverChanged)
    Q_PROPERTY(QString backupServerId READ backupServerId WRITE setBackupServerId NOTIFY failoverChanged)
    Q_PROPERTY(QString backupServerName READ backupServerName NOTIFY failoverChanged)
    Q_PROPERTY(bool randomOrder READ isRandomOrder WRITE setRandomOrder NOTIFY randomOrderChanged)
    Q_PROPERTY(int stuckTimeoutSeconds READ stuckTimeoutSeconds WRITE setStuckTimeoutSeconds NOTIFY stuckTimeoutSecondsChanged)
    Q_PROPERTY(int pauseSeconds READ pauseSeconds WRITE setPauseSeconds NOTIFY pauseSecondsChanged)
    Q_PROPERTY(QStringList hosts READ hosts WRITE setHosts NOTIFY hostsChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    // Счётчик успешных автореконнектов с момента запуска приложения (показывается на главном экране)
    Q_PROPERTY(int sessionReconnectCount READ sessionReconnectCount NOTIFY sessionReconnectCountChanged)
    Q_PROPERTY(QString lastReconnectTime READ lastReconnectTime NOTIFY sessionReconnectCountChanged)
    Q_PROPERTY(QString lastCheckDetails READ lastCheckDetails NOTIFY lastCheckDetailsChanged)

    // Event-log category toggles (checkboxes), applied live.
    Q_PROPERTY(bool logConnection READ logConnection WRITE setLogConnection NOTIFY logCategoriesChanged)
    Q_PROPERTY(bool logAutoReconnect READ logAutoReconnect WRITE setLogAutoReconnect NOTIFY logCategoriesChanged)
    Q_PROPERTY(bool logPingCheck READ logPingCheck WRITE setLogPingCheck NOTIFY logCategoriesChanged)
    Q_PROPERTY(bool logHostTest READ logHostTest WRITE setLogHostTest NOTIFY logCategoriesChanged)
    Q_PROPERTY(QString logFilePath READ logFilePath CONSTANT)

public:
    // FailMode values are also used from QML.
    enum FailMode {
        AllUnreachable = 0, // reconnect only when every host is unreachable (default)
        AnyUnreachable = 1  // reconnect when at least one host is unreachable
    };
    Q_ENUM(FailMode)

    // Event-log category bit flags.
    enum LogCategory {
        LogConnection    = 0x01, // manual/external connect & disconnect (user clicked)
        LogAutoReconnect = 0x02, // watchdog-triggered reconnects (with reason)
        LogPingCheck     = 0x04, // periodic ping-check results
        LogHostTest      = 0x08  // manual "Test" button results
    };
    Q_ENUM(LogCategory)

    explicit ReconnectController(ConnectionController *connectionController,
                                 ServersController *serversController,
                                 SecureAppSettingsRepository *appSettingsRepository,
                                 QObject *parent = nullptr);
    ~ReconnectController() override;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    int intervalSeconds() const;
    void setIntervalSeconds(int seconds);

    // Сколько проверок подряд должно провалиться, прежде чем переподключаться
    int failThreshold() const;
    void setFailThreshold(int count);

    int failMode() const;
    void setFailMode(int mode);

    bool isRandomOrder() const;
    void setRandomOrder(bool enabled);

    int stuckTimeoutSeconds() const;
    void setStuckTimeoutSeconds(int seconds);
    int pauseSeconds() const;
    void setPauseSeconds(int seconds);

    QStringList hosts() const;
    void setHosts(const QStringList &hosts);

    QString statusText() const;
    int sessionReconnectCount() const { return m_sessionReconnectCount; }
    QString lastReconnectTime() const { return m_lastReconnectTime; }
    QString lastCheckDetails() const;

    bool logConnection() const;
    void setLogConnection(bool enabled);
    bool logAutoReconnect() const;
    void setLogAutoReconnect(bool enabled);
    bool logPingCheck() const;
    void setLogPingCheck(bool enabled);
    bool logHostTest() const;
    void setLogHostTest(bool enabled);
    QString logFilePath() const;

    bool isFailoverEnabled() const;
    void setFailoverEnabled(bool enabled);
    QString backupServerId() const;
    void setBackupServerId(const QString &serverId);
    QString backupServerName() const;

public slots:
    // Список серверов для выбора резервного: «id», «название»
    QStringList serverIds() const;
    QStringList serverNames() const;
    // Convenience helpers for the QML list editor.
    void addHost(const QString &host);
    void removeHost(int index);
    // Runs a check right now (ignoring the timer); useful as a "Test" button.
    void checkNow();
    // Runs a standalone multi-packet ping against a single host and reports the result
    // (resolved IP, received/lost counts, timings) via hostTestFinished. Independent of the watchdog.
    void testHost(const QString &host);

    // Opens the event log in the system default text viewer / clears it.
    void openLogFile();
    void clearLog();

signals:
    void failoverChanged();
    void enabledChanged();
    void intervalSecondsChanged();
    void failThresholdChanged();
    void failModeChanged();
    void randomOrderChanged();
    void stuckTimeoutSecondsChanged();
    void pauseSecondsChanged();
    void hostsChanged();
    void statusTextChanged();
    void sessionReconnectCountChanged();
    void lastCheckDetailsChanged();
    void logCategoriesChanged();
    void hostTestStarted(const QString &host);
    void hostTestFinished(const QString &host, const QString &result);

private slots:
    void onTimerTimeout();
    void onConnectionStateChanged(Vpn::ConnectionState state);
    void onStuckTimeout();  // fired when an auto-reconnect hangs in a connecting state
    void onPauseTimeout();  // fired when the post-stuck pause elapses

private:
    void restartTimer();
    void startCheck();
    void pingNext();
    void onHostPingDone(int index, bool reachable, quint64 generation); // результат пинга одного хоста из рабочего потока
    void concludeCheck(bool healthy);
    void triggerReconnect();
    void startReconnectAttempt(); // disconnect if needed, then (re)open the connection
    // Меняет местами основной и резервный сервер. false — переключаться некуда.
    bool swapToBackup();
    void openReconnect();         // open the default server connection
    void setStatusText(const QString &text);
    void setLastCheckDetails(const QString &text);
    void logEvent(int category, const QString &message);
    void setLogCategory(int category, bool enabled);
    void cleanupPingProcess();
    void cleanupTestProcess();
    QString buildTestResult(const QString &host, const QString &output) const;

    ConnectionController *m_connectionController;
    ServersController *m_serversController;
    SecureAppSettingsRepository *m_appSettingsRepository;

    QTimer m_timer;

    // State for an in-flight check.
    bool m_checkInProgress = false;
    QStringList m_pendingHosts;
    int m_pendingIndex = 0;
    QList<QPair<QString, bool>> m_checkResults; // (host, reachable) in ping order, for the whole list

    // Параллельная проверка: каждый хост пингуется в своём потоке пула, результаты возвращаются в основной поток.
    quint64 m_checkGeneration = 0;  // увеличивается при отмене/завершении проверки, устаревшие результаты игнорируются
    int m_pendingCount = 0;         // сколько хостов ещё не ответили (или не истёк таймаут)
    int m_consecutiveFails = 0;     // проваленных проверок подряд
    qint64 m_graceUntilMs = 0;      // до этого момента (мс, epoch) проверки не выполняются (после подключения)
    int m_lastLoggedHealthy = -1;   // последнее записанное в лог состояние (для сокращения лога)
    qint64 m_lastHeartbeatMs = 0;   // когда в лог последний раз писали "пульс"

    // Standalone per-host "Test" ping, independent of the watchdog check above.
    QPointer<QProcess> m_testProcess;
    QTimer m_testKillTimer;
    bool m_testHandled = false;
    QString m_testHost;

    bool m_reconnectPending = false;
    bool m_autoReconnectActive = false; // true from an auto-reconnect trigger until it settles (for logging)
    QString m_lastUnreachableReason;    // reason string for the current auto-reconnect (for logging)

    // Recovery for a reconnect that hangs in a connecting state.
    QTimer m_stuckTimer;                 // single-shot; fires if connecting takes too long
    QTimer m_pauseTimer;                 // single-shot; the cooldown before the next retry
    bool m_pausePending = false;         // true while waiting out the post-stuck pause
    QString m_statusText;
    QString m_lastCheckDetails;

    int m_logCategories = 15; // bitmask of LogCategory

    int m_sessionReconnectCount = 0; // успешных автореконнектов за сессию
    QString m_lastReconnectTime;     // время последнего успешного автореконнекта (HH:mm:ss)
};

#endif // RECONNECTCONTROLLER_H
