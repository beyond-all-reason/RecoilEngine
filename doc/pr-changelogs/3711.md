 * Fix factory units surviving their factory's death during Lua `UnitCreated` callbacks. Cancel the new nanoframe if its factory dies or its build order changes before construction starts.
