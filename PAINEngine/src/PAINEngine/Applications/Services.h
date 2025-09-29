#pragma once

#ifndef SERVICES_HPP
#define SERVICES_HPP

#include <unordered_map>
#include <typeindex>
#include <memory>

namespace PAIN {

    class Services {
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

    private:
        std::unordered_map<std::type_index, std::weak_ptr<void>> map_;
    };
}

#endif
