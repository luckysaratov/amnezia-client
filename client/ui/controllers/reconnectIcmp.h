#ifndef RECONNECTICMP_H
#define RECONNECTICMP_H

#include <string>

// Системный ICMP-пинг для проверки туннеля автореконнектом (без запуска ping.exe).
// Реализован только для Windows (IcmpSendEcho); на других платформах всегда возвращает -1.
namespace reconnect_icmp
{
    // Возвращает: 1 - получен ответ, 0 - ответа нет, -1 - не поддерживается
    // (не IPv4, имя не разрешилось или платформа не Windows) - тогда вызывающий код может использовать ping.
    // Функция блокирующая, вызывать из рабочего потока.
    int pingIpv4(const std::string &host, int timeoutMs);
}

#endif // RECONNECTICMP_H
