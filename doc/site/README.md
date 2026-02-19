# Recoil Site

The recoil site is provisioned as a static site powered by [Hugo].

It also depends on and leverages several technologies and content:

- Dependency and task management with [mise]
- Lua API documentation extraction from engine source with [lua-doc-extractor]
- Templating of the Lua API content with [emmylua_doc_cli]
- Data from the engine binary to be templated as pages

## Local development (only static content)

> [!NOTE]
> All technical documentation from now on assumes your local directory is `doc/site`

Provisioning Hugo locally should enable development and testing of most content
in the site.

Data for provisioning the Lua API and engine provided pages should enable
testing the remaining pieces, but it's not required.

- Have `curl` binary available (not managed by mise)
- [Install mise]
- Run `mise run server`
- Navigate to the url displayed

It will take 2-5 seconds to generate the site (depending on hardware), but then any changes you make will be compiled quickly.

It's worth noting that since it's an incremental compile by default, if you start getting odd behavior, restart the server to build it fresh. This can happen a lot if you're messing with partials or layouts.

See [Writing Articles guide] for info on writing articles.

## Local development (full)

> [!WARNING]
> For dynamic content generation only WSL or Linux are supported

Required dependencies (not managed by mise): `curl`.

Run `mise run server_full`, alternatively see the next subsections on requirements and how to generate each of the dynamic content resources independently.

After full or target dynamic content has been generated, you only need to run `mise run server` unless you wish to regenerate dynamic content.

### Generating and Testing Lua Docs

- Run `mise run lua_pages`

See [Documenting Lua development guide] for info on writing documentation.

### Generating and Testing Engine binary Docs

Required dependencies (not managed by mise): `curl`.

- Run `mise run binary_pages`

## File Structure

These are the important directories:

- `.github`: Contains the workflow for publishing the site onto github pages.
- `content`: This is all of the markdown files that the site visitor will see.
  - `docs`: Documentation; Guides, API docs, etc.
    - `guides`: Guides for game developers.
      - `getting-started`: Guides for new users.
  - `development`: Guides and documentation for engine developers.
  - `articles`: General articles about the engine.
  - `changelogs`: Changelogs.
- `data`: A variety of config files used for rendering different parts of the website.
- `i18n`: Translations.
- `layouts`: Custom Hugo templates used for the site.
- `scripts`: Shell scripts used in the build process for github workflows.
- `generate_from_meta.py`: Glue code for generating lua docs from LLS meta files.
- `README.md`: You are here!

[Hugo]: https://gohugo.io/
[lua-doc-extractor]: https://github.com/rhys-vdw/lua-doc-extractor
[emmylua_doc_cli]: https://github.com/EmmyLuaLs/emmylua-analyzer-rust/tree/main/crates/emmylua_doc_cli
[mise]: https://mise.jdx.dev
[Install mise]: https://mise.jdx.dev/installing-mise.html
[Writing Articles guide]: content/development/writing-site-articles.md
[Documenting Lua development guide]: content/development/documenting-lua.md
