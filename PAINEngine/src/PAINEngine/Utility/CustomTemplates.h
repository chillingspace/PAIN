#pragma once

#ifndef CUSTOM_TEMPLATES_HPP
#define CUSTOM_TEMPLATES_HPP

#include <unordered_map>
#include <typeindex>
#include <memory>

namespace PAIN {
	namespace Custom {

        //Custom map class
        class ClassMap {
        public:
            template<class Iface>
            void set(std::shared_ptr<Iface> ptr) {
                static_assert(!std::is_pointer<Iface>::value, "Use the interface type, not a pointer");
                map_[std::type_index(typeid(Iface))] = std::move(ptr);
            }

            template<class Iface>
            std::shared_ptr<Iface> get() const {
                auto it = map_.find(std::type_index(typeid(Iface)));
                if (it == map_.end()) return {};
                return std::static_pointer_cast<Iface>(it->second);
            }

            template<class Iface, class Func>
            void forEachOfType(Func&& callback) const {
                for (const auto& pair : map_) {
                    auto casted = std::static_pointer_cast<Iface>(pair.second);
                    if (casted) {
                        callback(casted);
                    }
                }
            }

        private:
            std::unordered_map<std::type_index, std::shared_ptr<void>> map_;
        };

        //Custom map class
        class ClassWeakMap {
        public:
            template<class Iface>
            void set(std::shared_ptr<Iface> ptr) {
                static_assert(!std::is_pointer<Iface>::value, "Use the interface type, not a pointer");
                map_[std::type_index(typeid(Iface))] = std::move(ptr);
            }

            template<class Iface>
            std::shared_ptr<Iface> get() const {
                auto it = map_.find(std::type_index(typeid(Iface)));
                if (it == map_.end()) return {};
                return std::static_pointer_cast<Iface>(it->second.lock());
            }

            template<class Iface, class Func>
            void forEachOfType(Func&& callback) const {
                for (const auto& pair : map_) {
                    if (auto locked = pair.second.lock()) {
                        // This cast will succeed for any class that inherits from Iface
                        if (auto casted = std::static_pointer_cast<Iface>(locked)) {
                            callback(casted);
                        }
                    }
                }
            }

        private:
            std::unordered_map<std::type_index, std::weak_ptr<void>> map_;
        };
	}
}

#endif
