--
-- ls - list files
--

local getopt = require "getopt"

function space(n)
	local s = ""
	for i = 1, n, 1 do
		s = s .. " "
	end
	__print__(s)
end

__print__("\n")

-- Get the options
local long = false
local dirs = { }

for opt, arg in getopt(arg, 'l', dirs) do
	if opt == 'a' then
		long = true
    elseif opt == '?' then
        print('error: unknown option: ' .. arg)
        os.exit(1)
    elseif opt == ':' then
        print('error: missing argument: ' .. arg)
        os.exit(1)
    end
end

if #dirs == 0 then
	dirs[1] = "."
end

for _, dir in ipairs(dirs) do
	local root = wfs.open(dir)

	if not root then
		__print__("'", dir, "' dpes not exist\n")
	elseif not root:isdir() then
		__print__("'", dir, "' is not a directory\n")
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
end
