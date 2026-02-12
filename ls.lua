--
-- ls - list files
--

-- print(lfs.currentdir().."\n")
print("opening index.html\n")
file = io.open("/littlefs/swt.html", "r")
if file == nil then
    print("error opening file")
    return
end

print(file:read())
print(file:read())
print(file:read())
print(file:read())
print(file:read())
