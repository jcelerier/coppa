#pragma once

namespace coppa
{
/**
 * @brief The MessageType enum
 *
 * The potential common messages that can happen
 */
enum class MessageType
{
  Device,
  Namespace,
  PathChanged, PathAdded, PathRemoved, AttributesChanged,
  PathsChanged, PathsAdded, PathsRemoved, AttributesChangedArray,
};
}
