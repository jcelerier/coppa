#pragma once

namespace coppa
{
enum class MessageType
{
  Device,
  Namespace,
  PathChanged, PathAdded, PathRemoved, AttributesChanged,
  PathsChanged, PathsAdded, PathsRemoved, AttributesChangedArray,
};
}
