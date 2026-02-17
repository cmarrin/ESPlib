--
-- ls - list files
--

print("listing files in '/'\n")

print("********** cpl = ", __cpl__)

local root = wfs.open("/")

if not root:isdir() then
	print("'/' is not a directory'")
else
	local path = root:next_dir_path()
	if path then
		while true
		do
			local file = wfs.open(path)
			local info = file:isdir() and "dir" or file:size()
				
			print ("\t " .. file:name() .. "\t" .. info)

            path = root:next_dir_path();
            if (path == "") then
                break;
            end
		end
	end
end
