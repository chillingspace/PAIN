-- MUST return a table (BakeLuaFileToJson enforces this)
return {
  schema = 1,
  enemy = {
    name = "rubberduckduck",
    hp   = 25,
    pos  = { x = 3.5, y = -1.0 },
    waypoints = { {x=0,y=0}, {x=2,y=1}, {x=4,y=1} }
  }
}
