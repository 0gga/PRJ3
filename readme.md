# **NFC Reader System**<br>

> ## **Dependencies**<br>
>
>> Run `git submodule update --init --recursive --remote --merge` in root to clone external dependencies.
>
>> ### **Current dependencies:**
>>
>>- **boost.asio**  
>>  Read documentation for info on sub-dependencies  
>>- **nlohmann/json**

> ## **CLI Syntax & Cmdlets**<br>
>
>> ### **Cmdlets**<br>
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
>
>> ### **Syntax**<br>
>>
>>_**To make the CLI commands parseable, they must be syntactically correct:**_  
>> CLI command arguments must always be separated by a single space,  
>> and must never have any trailing spaces:  
>> `newUser john 3`
>>
>>User or door names must be either snake_case or camelCase:  
>> `rmUser johnDoe`  
>> `rmUser john_doe`  
>> The CLI parser will look for both and ensure all entries are removed.
>>
>>If exceptions are made such as PascalCase or PascalSnake_Case,  
>> it will still work as long as the preceding letter to a surname is lower case:  
>> `rmDoor JohnDoe`  
>> `rmDoor John_Doe`  
>> `rmDoor JOHN_Doe`
>>
>>The following combinations will not work:  
>> `newUser JOHNDOE`  
>> `newUser johNdoe`
>>
>>When editing a user/door specific syntax must be followed:
>>`mvUser 0gga ogga 1`
>>`mvUser door1 door2 1`
>> The 1st argument after the cmdlet is the user/door you wish to edit,
>> the 2nd is their new name,
>> the 3rd is their new accessLevel.
>
>>#### **Possible additions:**
>>
>>- Get user specific logs: `getULog 0gga`

> ## **Compilation**<br>
>> ### **Cross Compilation**<br>
>>CMake is used for compilation and works well with any toolchain.  
> > Use CLion WSL toolchain for easy cross compilation.

> ## **Features**<br>
>> ### **Implemented**<br>
>>- Easy Admin configuration through CLI connection on a separate port.
>>- Constantly accepts clients and stores information independently.
>>- Systemwide unified users.json - read onto hashmaps at startup for efficient runtime.
>>- String parser for standardized name format - snake_case.  
>> <br>
>>- ASIO - Asynchronous client handling.
>>- Asynchronous client handling runs on hardcoded amount of dedicated threads{4}.
>>- Callback to avoid blocking IO.
>>- Utilizes shared_lock & unique_lock for read/write respectively.
>> <br>
>>- OOP implementation for simple high-level usage.
>
>> ### **To be implemented**<br>
>>- The system currently doesn't write systemwide logs nor user logs,<br>
    albeit this could easily be implemented since clients are already assigned names.