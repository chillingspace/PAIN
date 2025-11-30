local log = printLog or log_info or print

log("hello from test.lua")

if registerUpdate then
  registerUpdate(function(dt) -- under luamanager
    log(("update tick dt=%.3f"):format(dt))
  end)
else
  log("no LuaManager present; doing a one-shot update") -- just luastate
  if _G.update then _G.update(0.016) end  -- optional, if you defined update()
end
