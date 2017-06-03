package = "WowLib"
version = "5.1.4-1"
source = {
   url = "http://luaforge.net/frs/download.php/4024/wowlib-5.1.4-1.tar.gz",
   md5 = "f9508aa9bbf59c3e6accce679b584464",
}
description = {
   summary = "A supplemental library that changes the Lua global environment to look like that provided by World of Warcraft",
   detailed = "This library re-defines string.format to accept Lua 4.0's position specifiers, and defines a few new functionsn that are provided by World of Warcraft.  In addition it sets a number of global aliases.",
   homepage = "http://wowlib.luaforge.net/",
   license = "MIT",
}
dependencies = {
   "lua >= 5.1",
}
build = {
   type = "make",
   install_variables = {
      LUA_DIR = "$(LUADIR)",
      LUA_LIBDIR = "$(LIBDIR)",
   },
}
