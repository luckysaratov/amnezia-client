#ifndef SPLITTUNNELINGEXTRAS_H
#define SPLITTUNNELINGEXTRAS_H

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QStringList>

// Вспомогательные функции для флажков раздельного туннелирования
// «Исключить российский трафик» и «Исключить локальный трафик».
// Работают только с IPv4-подсетями в формате CIDR.
namespace SplitTunnelingExtras
{
    // Локальные (частные) сети, которые исключаются из туннеля
    QStringList localNetworks();

    // Российские подсети: скачанный и сохранённый список, если он есть, иначе встроенный снимок
    QStringList russianSubnets();

    // Количество подсетей в используемом списке и дата его обновления (пустая дата — встроенный снимок)
    int russianSubnetsCount();
    QDateTime russianSubnetsUpdatedAt();

    // Разбор JSON ipverse/country-ip-blocks (prefixes.ipv4). false — если формат неверный
    bool parseRussianSubnetsJson(const QByteArray &json, QStringList &subnets);

    // Сохранение скачанного списка в каталог данных приложения
    bool saveRussianSubnets(const QStringList &subnets);

    // Вычитание: возвращает ranges без адресов из holes, объединённые и минимизированные.
    // Некорректные записи пропускаются.
    QStringList subtract(const QStringList &ranges, const QStringList &holes);

    // Итоговый список исключений для флажков. holes — адреса, которые должны остаться в туннеле
    // (адрес клиента, шлюз, DNS), чтобы исключение локальной сети не ломало сам туннель.
    QStringList buildExclusions(bool excludeRussia, bool excludeLocal, const QStringList &holes);

    // Разбор «a.b.c.d», «a.b.c.d/n». Возвращает false для неверного формата.
    bool parseCidr(const QString &text, quint32 &address, int &prefixLength);
}

#endif // SPLITTUNNELINGEXTRAS_H
