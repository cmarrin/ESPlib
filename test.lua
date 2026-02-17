--
-- Test Lua command
--
-- Tests the interface between the Web based shell and the Shell class on the server
--

file = wfs.open("/foo.txt", "w")

file:write("This is the first line of text\n")
file:write("This is the second line of text\n") 
file:write("This is the third line of text\n")
file:write("This is the fourth line of text\n") 
file:write("This is the fifth line of text\n")
file:write("This is the sixth line of text\n")
file:write("This is the seventh line of text\n")
file:write("This is the eighth line of text\n") 
file:write("This is the ninth line of text\n")

file:close()
