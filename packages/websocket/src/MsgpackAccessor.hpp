/*
 * Copyright (C) 2020 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef SOSS_WEBSOCKET__MSGPACKACCESSOR_HPP
#define SOSS_WEBSOCKET__MSGPACKACCESSOR_HPP

#include <soss/Message.hpp>

namespace soss {
namespace websocket {

class MsgpackAccessor {
public:

  template<typename T>
  static inline T get(const MsgpackMessage& msg, const std::string& key)
  {
    try
    {
      return msg.at(key).as<T>();
    }
    catch (const msgpack::type_error&)
    {
      throw std::bad_cast();
    }
  }
};

} // namespace websocket
} // namespace soss

#endif //SOSS_WEBSOCKET__MSGPACKACCESSOR_HPP
