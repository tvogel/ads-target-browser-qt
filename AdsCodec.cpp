#include "AdsCodec.h"

namespace Ads
{
QTextCodec * codec()
{
  static auto codec = QTextCodec::codecForName("Windows-1252");
  return codec;
}
} // namespace Ads
