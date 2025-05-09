# Setting Up Local Site Development

This version of the site was designed to be as simple to edit as possible. In the majority of cases, all you need is HUGO itself.

## Writing articles for the site

All you need is `hugo-extended`, available now at your local package manager.

`cd` into `doc/site` and run `hugo server`. It will take 2-5 seconds to generate the site, but then any changes you make will be compiled quickly. The site will be hosted on [Localhost](http://localhost:1313/RecoilEngine).

It's worth noting that since it's an incremental compile by default, if you start getting odd behavior, restart the server to build it fresh. This happens a lot if you're messing with partials or layouts.

## Editing Lua Docs

if you are editing the Lua docs, the process can be a little longwinded. They need to be extracted from the C++ files, compiled into markdown, and then moved.

Doing this locally requires:
1. Rust, for cargo
2. Node, for NPM (or equivalent software repo)
3. Python 3, latest version, for glue code

And the following programs:
1. Hugo extended, latest version, installed from your favorite package manager
2. Emmylua_doc_cli, 0.7.2 or greater, installed with cargo
3. lua-doc-extractor by resident rhys_vdw

Windows:
``` bash
scoop install hugo-extended
cargo install emmylua_doc_cli
npm install -g lua-doc-extractor
```

Linux:
``` bash
sudo apt-get install hugo-extended
cargo install emmylua_doc_cli
npm install -g lua-doc-extractor
```

To compile the site, `cd` into the `doc` directory and run `./build_site.sh`. Then, `cd` into `site` and run `hugo server`.

# File Structure

The file structure of the site is almost entirely the same as a generic HUGO site. Here's what you need to be interested in:

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
- `layouts`: Custom HUGO templates used for the site.
- `scripts`: Shell scripts used in the build process for github workflows.
- `generate_from_meta.py`: Glue code for generating lua docs from LLS meta files.
- `README.md`: You are here!

# Writing Articles

## Quickstart

This site is built using [HUGO]. Pretty much everything you need is on their website for how to make an article, but here's a brief overview:
1. `cd doc/site` if you're in the root directory of the Recoil repo.
2. `hugo new <filepath>/<filename>.md` will create a new page. For example, `hugo new content/docs/guides/writing-widgets.md`. For filenames and folder names especially, it tends to struggle with spaces, so avoid those. You can make the file manually, but the tool generates the head matter (page info section) for you.
3. Write your article.
4. When you are done, set `draft` to `false` in the head matter, otherwise it will not be rendered.

## Things To Know

- As previously mentioned, the theme we are using seems to struggle with spaces in filepaths, so avoid them.
- If you're making a new folder, it's best to make an `_index.md` for the folder.
- If you're writing a link within the site, use `{{% ref "path/to/page/" %}}` for it to render correctly in relation to the site root (`beyond-all-reason.github.io/RecoilEngine`), otherwise they will be in relation to `beyond-all-reason.github.io/`.

### Will my code be templated?

With the former Jekyll site, lua code would often be misinterpreted as templating code and cause issues, due to the `{{}}` syntax for nested tables. HUGO has a similar syntax with the curly braces. Does HUGO have the same problem?

Short answer: no. HUGO content files (the markdown) cannot have templates executed within them. They *do* have a limited form of templating called [shortcodes](https://gohugo.io/content-management/shortcodes/), and shortcodes have template code that is executed. These are executed when the markdown page is rendered into HTML. These have the syntax of:
Notation|Example
:--|:--
Markdown|`{{%/* foo */%}} ## Section 1 {{%/* /foo */%}}`
Standard|`{{</* foo */>}} ## Section 2 {{</* /foo */>}}`

Notice that both of these are invalid Lua syntax and won't come up naturally, so there is no risk of accidentally hitting it while writing docs.
Markdown notation shortcodes will render before the markdown is processed into HTML. The standard notation will render separately and merged into the page after markdown rendering, so any contents will remain untouched by the renderign process.

This does mean, however, that ad-hoc templating is difficult. This isn't an issue in the majority of cases, though.
For reference, the rendering process goes for each content file as follows:
1. Standard shortcodes are pulled from the content file (markdown).
2. Markdown shortcodes are rendered.
3. The content file is rendered into HTML.
4. Standard shorcodes are rendered, and merged into the document.
5. The template is rendered using the content document. Templates are usually defined with a section like:
```
<div class="content">
  {{ .Content }}
</div>
```
  This has the consequence that all template commands are processed *before* the content file goes in, so any template instructions within the content file are ignored. Thus, your code is safe.

## Shortcodes, Layouts, and Other Library

Along with the default shortcodes, you also have access to some more added by the site or our theme, Hextra.

### Hextra

- Our theme's shortcodes can be found [in their docs](https://imfing.github.io/hextra/docs/guide/shortcodes/)
- Hextra also allows use of the following:
  - Syntax Highlighting
  - LaTeX
  - Diagrams/flow charts

### Recoil Website

A few shortcodes and layouts have been written for the website specifically. They won't really be useful in most circumstances, but they are worth writing down.
- Shortcode `contributors` will render a list of contributors from Github. This is used for the site's homepage.
- Shortcode `latest_release` will render links for the latest release of Recoil, as well as download links and the releases page. This is also used for the site's homepage.
- Layout `commands` will render out a list of chat commands from a provided data file in the `data` directory of the site. This is used for the [Unsynced](docs/unsynced-commands) and [Synced](docs/synced-commands) pages. It expects a JSON file with the following schema:
```json
{
  "command-name": {
    "arguments": {
      "": "Default command description",
      "arguments": "Action description"
    },
    "cheatRequired": false,
    "command": "CommandName",
    "description": "'twas brillig, and the slithy toves"
  }
}
```
  Commands also requires the following info in the head matter:
  - `params.context`: the name of the data to pull from (which is the filepath of the data file relative to the data folder without the extension). For example: `context = "synced_commands"`.
  - `layout = "commands"`
  - `type = "docs"`

[HUGO]: https://gohugo.io/
