--
-- ls - list files
--

print("opening index.html\n")
file = wfs.open("/Wikimedia-2025.txt", "r")
if file == nil then
    print("error opening file")
    return
end

print(file:read())
print(file:read())
print(file:read())
print(file:read())
print(file:read())
