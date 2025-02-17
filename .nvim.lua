require('telescope').setup {
    defaults = {
        file_ignore_patterns = {
            "%.git/*",           -- Ignore .git directory
            "node_modules/*",    -- Ignore node_modules
            "%.log$",            -- Ignore log files
            "%.tmp$",            -- Ignore temporary files
        },
    },
    pickers = {
        live_grep = {
            search_dirs = { "src", "/res/shaders", "extern" },
        },
    },
}

require('dap').configurations.cpp = {
  {
    name = "Launch RabbitHole",
    type = "lldb",
    request = "launch",
    program = function()
      return vim.fn.getcwd() .. '/build/rabbithole'
    end,
    cwd = '${workspaceFolder}', -- The root folder of your project
    stopOnEntry = false,        -- Start running immediately
    args = {},                  -- Add any program arguments here
    env = {},                   -- Add any environment variables here
    sourceLanguages = { "cpp", "c" }, -- Associate these languages for debugging
    sourceMap = {
      ["${workspaceFolder}/build"] = "${workspaceFolder}/src",
    },
  },
}


