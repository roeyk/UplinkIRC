#pragma once
#include <QHostAddress>

// Returns true for loopback, link-local, and RFC-1918 private ranges.
// Used by linkpreview (SSRF guard) and dccreceive (peer validation).
inline bool isPrivateAddress(const QHostAddress &a)
{
    if (a.isNull())      return true;
    if (a.isLoopback())  return true;
    if (a.isMulticast()) return true;
    if (a.isInSubnet(QHostAddress("10.0.0.0"),    8))   return true;
    if (a.isInSubnet(QHostAddress("172.16.0.0"),  12))  return true;
    if (a.isInSubnet(QHostAddress("192.168.0.0"), 16))  return true;
    if (a.isInSubnet(QHostAddress("169.254.0.0"), 16))  return true;
    if (a.isInSubnet(QHostAddress("127.0.0.0"),   8))   return true;
    if (a.isInSubnet(QHostAddress("::1"),         128)) return true;
    if (a.isInSubnet(QHostAddress("fc00::"),      7))   return true;
    if (a.isInSubnet(QHostAddress("fe80::"),      10))  return true;
    return false;
}
