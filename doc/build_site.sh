cd ../
lua-doc-extractor rts/Lua/*.cpp rts/Rml/SolLua/*.cpp --dest rts/Lua/library/generated
emmylua_doc_cli -i rts/Lua/library -o doc/temp --override-template doc/emmylua-doc-cli-template
python3 doc/site/generate_from_meta.py doc/meta doc/temp doc/site/content/docs/lua-api
