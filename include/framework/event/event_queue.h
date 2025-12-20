//
// Created by Niffoxic (Aka Harsh Dubey) on 12/18/2025.
//
// -----------------------------------------------------------------------------
// Project   : DirectX12
// Purpose   : Academic and self-learning computer graphics project.
// Codebase  : DirectX 12 implementation based on the Luna Graphics Programming
//             textbook. This repository includes practical scene-style
//             implementations as well as answers and solutions to the
//             end-of-chapter questions for learning and reference purposes.
// License   : MIT License
// -----------------------------------------------------------------------------
//
// Copyright (c) 2025 Niffoxic
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------------

#ifndef DIRECTX12_EVENT_QUEUE_H
#define DIRECTX12_EVENT_QUEUE_H

#include <sal.h>
#include <unordered_map>
#include <functional>
#include <ranges>
#include <typeindex>
#include <vector>

namespace framework
{
    //~ For Every Type
    template<typename EventT>
    struct Channel
    {
        using Callback = std::function<void(_In_ const EventT&)>;

        inline static std::vector<Callback> Subscribers{};   // subscribers
        inline static std::vector<EventT>   Queue{};  // pending events
    };

    //~ Subscription token
    struct SubToken
    {
        std::type_index type{ typeid(void) };
        std::size_t     index{ static_cast<std::size_t>(-1) };
        bool            valid{ false };

        size_t operator()() const
        {
            return index;
        }
    };

    //~ Event queue Facade
    class EventQueue
    {
        struct TypeOps
        {
            void (*dispatch)();                    // calls DispatchType<T>()
            void (*clear)();                       // clears Channel<T>::Queue
            bool (*unsubscribe)(_In_ std::size_t); // clears a subscriber slot
        };

    public:
        template<typename EventT>
        _Ret_valid_ static SubToken Subscribe(_In_ Channel<EventT>::Callback cb)
        {
            RegisterIfNeeded<EventT>(); // first time stuff

            auto& subs = Channel<EventT>::Subscribers; // respective channel
            subs.push_back(std::move(cb));

            return { std::type_index(typeid(EventT)), subs.size() - 1, true };
        }

        template<typename EventT>
        static void Post(_In_ const EventT& event)
        {
            RegisterIfNeeded<EventT>();
            Channel<EventT>::Queue.push_back(event);
        }

        static void DispatchAll()
        {
            for (const auto &val: s_mapRegistry | std::views::values) val.dispatch();
        }

        static void ClearAll()
        {
            for (const auto &val: s_mapRegistry | std::views::values) val.clear();
        }

        static void Unsubscribe(_Inout_ SubToken& sub)
        {
            if (!sub.valid) return;

            auto& reg = s_mapRegistry;
            if (reg.contains(sub.type))
            {
                reg[ sub.type ].unsubscribe(sub.index);
            }

            sub.valid = false;
        }

        //~ If wanted to dispatch single known type only
        template<typename EventT>
        static void DispatchType()
        {
            auto& queue = Channel<EventT>::Queue;
            auto& subscribers = Channel<EventT>::Subscribers;

            for (std::size_t i = 0; i < queue.size(); ++i)
            {
                const EventT& event = queue[ i ];
                for (std::size_t subscriber = 0; subscriber < subscribers.size(); subscriber++)
                {
                    auto& callbackFn = subscribers[ subscriber ];
                    if (callbackFn) callbackFn(event);
                }
            }

            queue.clear();
        }

    private:
        template<typename EventT>
        static void DispatchThunk() { DispatchType<EventT>(); }

        template<typename EventT>
        static void ClearThunk() { Channel<EventT>::Queue.clear(); }

        template<typename EventT>
        _Success_(return)
        static bool UnsubThunk(_In_ std::size_t idx)
        {
            auto& subscribers = Channel<EventT>::Subscribers;

            if (idx >= subscribers.size()) return false;

            subscribers[ idx ] = nullptr; // just ignore if nullptr saves realloc
            return true;
        }

        template<typename EventT>
        static void RegisterIfNeeded()
        {
            auto& reg = s_mapRegistry;
            const std::type_index key(typeid(EventT)); // Handle

            if (reg.contains(key)) return;

            // Initialize type operations
            const TypeOps op =
            {
                &DispatchThunk<EventT>,
                &ClearThunk<EventT>,
                &UnsubThunk<EventT>
            };
            reg[ key ] = op;
        }

    private:
        static std::unordered_map<std::type_index, TypeOps> s_mapRegistry;
    };
} // namespace framework

#endif //DIRECTX12_EVENT_QUEUE_H
