
// Copyright (c) 2018 brinkqiang (brink.qiang@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "luago_module.h"
#include "sol.hpp"
#include "coroutine.h"

namespace lua_module_luago
{
    static sol::table require_api(sol::this_state L)
    {
        sol::state_view lua(L);

        lua.open_libraries(sol::lib::base, sol::lib::coroutine);

        sol::table module = lua.create_table();

        module.set_function("run", [](sol::object obj, sol::this_state L) {
            if (obj.get_type() != sol::type::function)
            {
                return;
            }

            auto func = obj.as<sol::protected_function>();
            auto result = func();
            if (!result.valid())
            {
                sol::error ec = result;
                printf("func.call() %s\n", ec.what());
            }
        });
        co::Scheduler& mainScheduler = co::Scheduler::getInstance();
        mainScheduler.goStart(1, 1);
        module.set("__MAIN_SCHED", &mainScheduler);

        auto mainSched = lua.get<co::Scheduler*>("__MAIN_SCHED");
        module.set_function("go", [mainSched](sol::object obj, sol::this_state L) {
            if (obj.get_type() != sol::type::function)
            {
                return;
            }

            sol::thread thread = sol::thread::create(L);
            sol::coroutine* coFunc = new sol::coroutine(thread.state(), obj);

            go_stack(1024) co_scheduler(mainSched)[coFunc, thread] {
                sol::protected_function_result result = coFunc->call();
                if (!result.valid()) {
                    sol::error ec = result;
                    printf("func.call() %s\n", ec.what());
                }
                delete coFunc;
            };
        });
        module.set_function("yield", [](){ 
            co_yield;
        });
        module.set_function("sleep", [](uint64_t t){ 
            co_sleep(t);
        });
        return module;
    }
} // namespace lua_module_luago

LUAMOD_API int luaopen_luago(lua_State *L)
{
    return sol::stack::call_lua(L, 1, lua_module_luago::require_api);
}

LUAMOD_API int require_luago(lua_State *L)
{
    luaL_requiref(L, "luago", luaopen_luago, 0);
    printf("lua module: require luago\n");
    return 1;
}