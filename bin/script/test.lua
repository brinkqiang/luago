
local luago = require("luago")
luago.run( 
    function()
        print("luago.run hello world")
    end
)

luago.go( 
    function()
        print("luago.go hello world")
    end
)