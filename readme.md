# **NFC Reader System**<br>

This readme mainly describes:

- Core features of the server
- How to handle dependencies
- Syntax for all cmdlets
- Compilation information

> ## **Dependencies**<br>
>
>> ### **Server dependencies:**
>>
>>- **boost.asio** - Read documentation for info on sub-dependencies
>>- **nlohmann/json**
>
>> ### **CLI & Client dependencies:**
>>
>>- **wiringPi** - Must be installed to RPi root:<br>
>>
>> Grab newest release here:<br>
> > https://github.com/WiringPi/WiringPi/releases<br>
> > copy it to `<pi>@<rpi>:~` with scp <br>
>> ```shell
>> scp -r "C:\Users\USERNAME\Downloads\wiringpi-3.x.deb" <pi>@<ip>:/home/<pi>/
>> ```
> > SSH into RPi and:
>>
>> ```shell
>> sudo apt install ./wiringpi-3.x.deb
>> ```
>
>> Run in root to clone external dependencies.
>> ```shell
>> git submodule update --init --recursive
>> ```
>
>> <h3>Addition/Removal of dependencies:</h3>
>> <details>
>> <summary>Click to expand</summary>
>>
>> <h4>To add a submodule:</h4>
>>
>> ```shell
>> git submodule add url extern/<name>
>> git submodule update --init --recursive --remote --merge
>> ```
>>
>> <h4>To remove a submodule:</h4>
>>
>> ```shell
>> git rm -r extern/<name>
>> git add .gitmodules
>> git config -f .git/config --remove-section submodule.extern/<name>
>> ```
>>
>>> Run one of the following
>>>
>>> <h4>Bash:</h4>
>>>
>>> ```shell
>>> rm -rf .git/modules/extern/<name>
>>> ```
>>>
>>> <h4>PowerShell:</h4>
>>>
>>> ```shell
>>> Remove-Item .git/modules/extern/<name> -Recurse -Force
>>> ```
>></details>
>>
>> <h3>In case of FUBAR:</h3>
>> <details>
>> <summary>Click to expand</summary>
>>
>> Run in root to reset .gitmodules after removal from .gitmodules.
>>
>>```shell
>>rm -rf .git/modules/*
>>git rm -r --cached .
>>```
>>> **Bash**
>>> ```shell
>>> git config -f .gitmodules --get-regexp submodule | \
>>> awk '{
>>> if ($1 ~ /\.path$/) path=$3;
>>> if ($1 ~ /\.url$/)  url=$3;
>>> if (path && url) {
>>> print "git submodule add " url " " path;
>>> path=""; url="";
>>> }
>>> }'
>>> ```
>>> **PowerShell**
>>> ```shell
>>> $modules = git config -f .gitmodules --get-regexp submodule
>>> 
>>> $path = ""
>>> $url = ""
>>> 
>>> foreach ($line in $modules) {
>>> $parts = $line -split "\s+"
>>> 
>>>     if ($parts[0] -like "*.path") {
>>>         $path = $parts[1]
>>>     }
>>> 
>>>     if ($parts[0] -like "*.url") {
>>>         $url = $parts[1]
>>>     }
>>> 
>>>     if ($path -and $url) {
>>>         Write-Host "Adding submodule: $url -> $path"
>>>         git submodule add $url $path
>>>         $path = ""
>>>         $url = ""
>>>     }
>>> }
>>> Run in root to initialize and clone .gitmodules again.
>>> ```shell
>>> git submodule init
>>> git submodule update --init --recursive --remote --merge
>>> ```
>></details>

> ## **CLI Syntax & Cmdlets**<br>
>
>> <h3>Cmdlets:</h3>
>> <details>
>> <summary>Click to expand</summary>
>>
>>- Add a new door by name: `newDoor <string>door1 <int>accessLevel`
>>- Create a new user: `newUser <string>0gga <int>accessLevel`
>>- Remove an existing door: `rmDoor <string>door1`
>>- Remove an existing user: `rmUser <string>0gga`
>>- Edit an existing door: `mvDoor <string>door1 <int>accessLevel`
>>- Edit an existing door: `mvDoor <string>0gga <int>accessLevel`
>>- Exit and kill the CLI connection: `exit`
>>- Shutdown the entire system: `shutdown`
>>- Get date-specific system logs: `getSystemLog <string>Date`
>>- Get user-specific logs: `getUserLog <string>0gga`
>>- Get door-specific logs: `getDoorLog <string>Door1`
>>
>> </details>
>
>> <h3>Syntax:</h3>
>> <details>
>> <summary>Click to expand</summary>
>>
>> <h4>To make the CLI commands parseable, they must be syntactically correct:</h4>
> > CLI command arguments must always be separated by a single space,
> > and must never have any trailing spaces:<br>
> > `newUser john 3`<br>
>>
>> User or door names must be either snake_case or camelCase:<br>
> > `rmUser johnDoe`<br>
> > `rmUser john_doe`<br>
>>
>> The CLI parser will look for both and ensure all entries are removed.<br>
> > If exceptions are made such as PascalCase or PascalSnake_Case,<br>
> > it will still work as long as the preceding letter to a surname is lower case:<br>
> > `rmDoor JohnDoe`<br>
> > `rmDoor John_Doe`<br>
> > `rmDoor JOHN_Doe`
>>
>> The following combinations will not work:<br>
> > `newUser JOHNDOE`<br>
> > `newUser johNdoe`<br>
>>
>> When editing a user/door it is important to understand that,
> > all users are permanently mapped to their UID.<br>
> > When editing a user/door specific syntax must be followed:<br>
> > `mvUser 0gga ogga 1`<br>
> > `mvDoor door1 door2 1`<br>
>>- 1st argument is the user/door you wish to edit,
>>- 2nd argument is their new name. (Old name can be reused)
>>- 3rd argument is their new accessLevel.
>>
>> If only the user-/doorname wishes to be edited the 3rd argument can be omitted as such:<br>
> > `mvUser 0gga ogga`<br>
> > `mvDoor door1 door2`<br>
>> </details>
>
>> #### **Possible additions:**
>>
>>- Get user specific logs: `getULog 0gga`

> ## **Compilation**<br>
>> ### **Cross Compilation**<br>
>> CMake is used for compilation and works well with all default CLion integrated toolchains.<br>
> > Only the Server can be cross compiled since the CLI and Client rely on
> > UNIX specific libraries & wiringPi, which uses RPi-specific hardware registers.<br>
> > To cross compile the server it is recommended to use CLions default WSL toolchain.<br>
>>
>> ### **CMake options**<br>
>> To enable args and/or debug output the following flags must be set as CMake options before building CMake config:<br>
> > `DDEBUG=1`,
> > `DARGS=1`
>>

> ## **Features**<br>
>> ### **Implemented**<br>
>>- Easy Admin configuration through CLI connection on a separate port.
>>- Constantly accepts clients and stores information independently.
>>- Systemwide unified users.json - read onto hashmaps at startup for efficient runtime.
>>- String parser for standardized name format - snake_case.<br>
>>
>>
>>- ASIO - Asynchronous client handling.
>>- Asynchronous client handling runs on hardcoded amount of dedicated threads{4}.
>>- Callback to avoid blocking IO.
>>- Utilizes shared_lock & unique_lock for read/write respectively.
>>
>>
>>- OOP implementation for easy API usage.