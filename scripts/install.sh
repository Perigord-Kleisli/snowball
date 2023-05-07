#!/usr/bin/env bash
set -e


SNOWBALL_INSTALL_DIR=~/.snowball

OS=$(uname -s | awk '{print tolower($0)}')
ARCH=$(uname -m)

LIB_FOLDER="/usr/lib"
YES="$1"

if [ "$OS" != "linux" ] && [ "$OS" != "darwin" ]; then
  echo "error: Pre-built binaries only exist for Linux and macOS." >&2
  exit 1
fi

SNOWBALL_BUILD_ARCHIVE=snowball-$OS-$ARCH.tar.gz

mkdir -p $SNOWBALL_INSTALL_DIR
cd $SNOWBALL_INSTALL_DIR
curl -L https://github.com/snowball-lang/snowball/releases/latest/download/"$SNOWBALL_BUILD_ARCHIVE" | tar zxvf - --strip-components=1

if [[ "$OSTYPE" == "darwin"* ]]; then

    sudo mv libSnowballRuntime.dylib $LIB_FOLDER
    sudo mv libSnowball.dylib $LIB_FOLDER
else
    sudo mv libSnowballRuntime.so $LIB_FOLDER
    sudo mv libSnowball.so $LIB_FOLDER
fi

EXPORT_COMMAND="$(pwd)/bin"
echo "PATH export command:"
echo "  $EXPORT_COMMAND"

# function to check if a file exists and is writable
check_file_writable() {
    local file="$1"
    if [[ -e "$file" ]]; then
        if [[ -w "$file" ]]; then
            return 0
        else
            echo "Error: $file is not writable"
            return 1
        fi
    else
        return 1
    fi
}

# function to prompt the user for input
prompt_user() {
    local message="$1"
    local default_value="$2"
    local allowed_values="$3"
    
    read -p "$message" -i "$default_value" -e value
    if [[ -n "$allowed_values" ]]; then
        if [[ "$allowed_values" == *"$value"* ]]; then
            echo "$value"
        else
            echo "$default_value"
        fi
    else
        echo "$value"
    fi
}

# function to add the command to the PATH in the config file
add_command_to_path() {
    local config_file="$1"

    if grep -q "$EXPORT_COMMAND" "$config_file"; then
        echo "PATH already updated in $config_file; skipping update."
        return
    fi

    read -p "Do you want to add $EXPORT_COMMAND to PATH in $config_file? [y/n]: " add_to_path
    if [[ "$add_to_path" == "y" || "$YES" == "-y" ]]; then
        echo "Updating $config_file ..."
        echo "" >> "$config_file"
        echo "export PATH=\"\$PATH:$EXPORT_COMMAND\"" >> "$config_file"
    else
        echo "Skipping update of $config_file."
    fi
}

# function to update the appropriate config file based on the shell
update_config_file() {
    local shell="$1"

    case "$shell" in
        *zsh)
            local zshrc="$HOME/.zshrc"
            local zshenv="$HOME/.zshenv"
            if check_file_writable "$zshrc"; then
                add_command_to_path "$zshrc"
            elif check_file_writable "$zshenv"; then
                add_command_to_path "$zshenv"
            else
                echo "Error: Cannot find a writable zsh config file" 1>&2
                exit 1
            fi
            ;;
        *bash)
            local bashrc="$HOME/.bashrc"
            local bash_profile="$HOME/.bash_profile"
            if check_file_writable "$bash_profile"; then
                add_command_to_path "$bash_profile"
            elif check_file_writable "$bashrc"; then
                add_command_to_path "$bashrc"
            else
                echo "Error: Cannot find a writable bash config file" 1>&2
                exit 1
            fi
            ;;
        *)
            echo "Error: Unknown shell type $shell" 1>&2
            exit 1
            ;;
    esac
}

shell="$SHELL"
if [[ -z "$shell" ]]; then
    shell=$(ps -p $$ -o args= | awk '{print $1}')
fi

update_config_file "$shell"

echo "Snowball successfully installed at: $(pwd)"
echo "Open a new terminal session or update your PATH to use snowball"

echo "Happy coding! 🐱"