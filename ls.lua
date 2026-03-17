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
        __print__('error: unknown option: ' .. arg .. "\n")
        os.exit(1)
    elseif opt == ':' then
        __print__('error: missing argument: ' .. arg .. "\n")
        os.exit(1)
    end
end

if #dirs == 0 then
	dirs[1] = "."
end

for _, dir in ipairs(dirs) do
	local root = wfs.open(dir)

	if not root:isdir() then
		__print__("'", dir, "' does not exist\n")
	elseif not root:isdir() then
		__print__("'", dir, "' is not a directory\n")
	else
		-- show the directory name
		__print__(dir..":\n")
		
		local FilenameWidth <const> = 34

		local lineWidth = 0;
		
		while true do
			local f = root:open_next_file()
			local filename = f:filename()
			if filename:len() == 0 then
				break
			end
			local dir = f:isdir()
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
			
			if dir then
				local s = "\x1b[33m"..filename.."/\x1b[0m"
				__print__(s)
			else
				__print__(filename)
			end
			space(extraSpace)
		end
		__print__("\n")
	end
end
