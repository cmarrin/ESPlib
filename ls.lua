--
-- ls - list files
--

function space(n)
	local s = ""
	for i = 1, n, 1 do
		s = s .. " "
	end
	__print__(s)
end

__print__("\n")

local root = wfs.open("/")

if not root:isdir() then
	__print__("'/' is not a directory'\n")
else
	local FilenameWidth <const> = 34
	
	local path = root:next_dir_path()
	
	local lineWidth = 0;
	
	if path then
		while true
		do
			local file = wfs.open(path)
			local dir = file:isdir()
			local filename = file:name()
			local extraSpace = FilenameWidth - filename:len()
			
			if dir then
				extraSpace = extraSpace - 1
			end
			
			-- Will this filename fit on this line?
			if lineWidth + FilenameWidth >= __cpl__ then
				__print__("\n")
				lineWidth = 0
			end
			
			lineWidth = lineWidth + FilenameWidth
			
			__print__(filename)
			if dir then
				__print__("/")
			end
			space(extraSpace)

            path = root:next_dir_path();
            if (path == "") then
                break;
            end
		end
		__print__("\n\n")
	end
end
