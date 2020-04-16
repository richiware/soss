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

#include <soss/json/conversion.hpp>
#include "BsonSerializer.hpp"

namespace soss {
namespace websocket {

MessagePtrT BsonSerializer::serialize(ConMsgManagerPtrT& con_msg_mgr, const nlohmann::json& msg) {
  auto out = nlohmann::json::to_bson(msg);
  auto ws_msg = con_msg_mgr->get_message();
  ws_msg->set_payload(out.data(), out.size());
  ws_msg->set_opcode(opcode);
  return ws_msg;
}

nlohmann::json BsonSerializer::deserialize(const std::string& data) {
  return nlohmann::json::from_bson(data);
}

} // namespace websocket
} // namespace soss