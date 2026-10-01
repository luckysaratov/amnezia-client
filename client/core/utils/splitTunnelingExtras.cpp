#include "splitTunnelingExtras.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QStringList>

#include <algorithm>
#include <utility>
#include <vector>

namespace
{
    using Interval = std::pair<quint64, quint64>; // [начало, конец] включительно

    const char kBuiltinResource[] = ":/split_tunneling/ru_subnets.txt";

    QString cacheFilePath()
    {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dir.isEmpty()) {
            return QString();
        }
        return QDir(dir).filePath(QStringLiteral("ru_subnets.txt"));
    }

    QString toCidr(quint32 address, int prefixLength)
    {
        return QStringLiteral("%1.%2.%3.%4/%5")
                .arg((address >> 24) & 0xff).arg((address >> 16) & 0xff)
                .arg((address >> 8) & 0xff).arg(address & 0xff)
                .arg(prefixLength);
    }

    bool toInterval(const QString &text, Interval &out)
    {
        quint32 address = 0;
        int prefix = 0;
        if (!SplitTunnelingExtras::parseCidr(text, address, prefix)) {
            return false;
        }
        const quint64 size = quint64(1) << (32 - prefix);
        const quint64 start = quint64(address) & ~(size - 1);
        out = { start, start + size - 1 };
        return true;
    }

    std::vector<Interval> mergeIntervals(std::vector<Interval> v)
    {
        std::sort(v.begin(), v.end());
        std::vector<Interval> merged;
        for (const Interval &i : v) {
            if (!merged.empty() && i.first <= merged.back().second + 1) {
                merged.back().second = std::max(merged.back().second, i.second);
            } else {
                merged.push_back(i);
            }
        }
        return merged;
    }

    // Преобразует интервал в минимальный набор CIDR
    void intervalToCidrs(quint64 start, quint64 end, QStringList &out)
    {
        while (start <= end) {
            // максимальный размер блока, выровненный по start
            int bits = 32;
            if (start != 0) {
                bits = 0;
                while (bits < 32 && ((start >> bits) & 1) == 0) {
                    ++bits;
                }
            }
            // уменьшаем блок, пока он не поместится в интервал
            while (bits > 0 && (start + (quint64(1) << bits) - 1) > end) {
                --bits;
            }
            out.append(toCidr(quint32(start), 32 - bits));
            start += quint64(1) << bits;
        }
    }

    QStringList readSubnetsFile(const QString &path)
    {
        QStringList result;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return result;
        }
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }
            quint32 a;
            int p;
            if (SplitTunnelingExtras::parseCidr(line, a, p)) {
                result.append(line);
            }
        }
        return result;
    }
}

namespace SplitTunnelingExtras
{
    bool parseCidr(const QString &text, quint32 &address, int &prefixLength)
    {
        const QString t = text.trimmed();
        const QStringList parts = t.split('/');
        if (parts.size() > 2) {
            return false;
        }
        const QStringList octets = parts.first().split('.');
        if (octets.size() != 4) {
            return false;
        }
        quint32 value = 0;
        for (const QString &o : octets) {
            bool ok = false;
            const int n = o.toInt(&ok);
            if (!ok || n < 0 || n > 255) {
                return false;
            }
            value = (value << 8) | quint32(n);
        }
        int prefix = 32;
        if (parts.size() == 2) {
            bool ok = false;
            prefix = parts.at(1).toInt(&ok);
            if (!ok || prefix < 0 || prefix > 32) {
                return false;
            }
        }
        address = value;
        prefixLength = prefix;
        return true;
    }

    QStringList localNetworks()
    {
        return { QStringLiteral("10.0.0.0/8"), QStringLiteral("172.16.0.0/12"), QStringLiteral("192.168.0.0/16") };
    }

    QStringList russianSubnets()
    {
        const QString cache = cacheFilePath();
        if (!cache.isEmpty() && QFileInfo::exists(cache)) {
            const QStringList fromCache = readSubnetsFile(cache);
            // слишком короткий список считаем повреждённым
            if (fromCache.size() > 1000) {
                return fromCache;
            }
        }
        return readSubnetsFile(QString::fromLatin1(kBuiltinResource));
    }

    int russianSubnetsCount()
    {
        return russianSubnets().size();
    }

    QDateTime russianSubnetsUpdatedAt()
    {
        const QString cache = cacheFilePath();
        if (!cache.isEmpty() && QFileInfo::exists(cache)) {
            return QFileInfo(cache).lastModified();
        }
        return QDateTime();
    }

    bool parseRussianSubnetsJson(const QByteArray &json, QStringList &subnets)
    {
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            return false;
        }
        const QJsonValue v4 = doc.object().value(QStringLiteral("prefixes")).toObject().value(QStringLiteral("ipv4"));
        if (!v4.isArray()) {
            return false;
        }
        std::vector<Interval> intervals;
        for (const QJsonValue &v : v4.toArray()) {
            Interval i;
            if (v.isString() && toInterval(v.toString(), i)) {
                intervals.push_back(i);
            }
        }
        // защита от пустого или обрезанного ответа
        if (intervals.size() < 1000) {
            return false;
        }
        QStringList result;
        for (const Interval &i : mergeIntervals(intervals)) {
            intervalToCidrs(i.first, i.second, result);
        }
        subnets = result;
        return true;
    }

    bool saveRussianSubnets(const QStringList &subnets)
    {
        const QString path = cacheFilePath();
        if (path.isEmpty()) {
            return false;
        }
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        file.write("# Российские IPv4-подсети (ipverse/country-ip-blocks), скачано приложением\n");
        file.write(subnets.join('\n').toUtf8());
        file.write("\n");
        return file.commit();
    }

    QStringList subtract(const QStringList &ranges, const QStringList &holes)
    {
        std::vector<Interval> base;
        for (const QString &r : ranges) {
            Interval i;
            if (toInterval(r, i)) {
                base.push_back(i);
            }
        }
        std::vector<Interval> cut;
        for (const QString &h : holes) {
            Interval i;
            if (toInterval(h, i)) {
                cut.push_back(i);
            }
        }
        base = mergeIntervals(base);
        cut = mergeIntervals(cut);

        QStringList result;
        size_t ci = 0;
        for (const Interval &b : base) {
            quint64 start = b.first;
            const quint64 end = b.second;
            // пропускаем дырки, которые закончились до начала текущего диапазона
            while (ci < cut.size() && cut[ci].second < start) {
                ++ci;
            }
            size_t cj = ci;
            while (cj < cut.size() && cut[cj].first <= end) {
                if (cut[cj].first > start) {
                    intervalToCidrs(start, cut[cj].first - 1, result);
                }
                start = std::max(start, cut[cj].second + 1);
                if (start > end) {
                    break;
                }
                ++cj;
            }
            if (start <= end) {
                intervalToCidrs(start, end, result);
            }
        }
        return result;
    }

    QStringList buildExclusions(bool excludeRussia, bool excludeLocal, const QStringList &holes)
    {
        QStringList all;
        if (excludeLocal) {
            all += localNetworks();
        }
        if (excludeRussia) {
            all += russianSubnets();
        }
        return subtract(all, holes);
    }
}
