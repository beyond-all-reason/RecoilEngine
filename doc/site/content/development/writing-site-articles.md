+++
title = 'Writing Site Articles'
date = 2025-05-14T19:17:01-07:00
draft = false
author = 'Slashscreen'
+++

## Quickstart

This site is built using [Hugo]. Pretty much everything you need is on their website for how to make an article, but here's a brief overview:

1. `cd doc/site` if you're in the root directory of the Recoil repo.
2. `hugo new <filepath>/<filename>.md` will create a new page. For example, `hugo new content/docs/guides/writing-widgets.md`. For filenames and folder names especially, it tends to struggle with spaces, so avoid those. You can make the file manually, but the tool generates the head matter (page info section) for you.
3. Write your article.
4. When you are done, set `draft` to `false` in the head matter, otherwise it will not be rendered.
5. `hugo server`

It's worth noting that since it's an incremental compile by default, if you start getting odd behavior, restart the server to build it fresh. This can happen a lot if you're messing with partials or layouts.

## Things To Know

- As previously mentioned, the theme we are using seems to struggle with spaces in filepaths, so avoid them.
- If you're making a new folder, it's best to make an `_index.md` for the folder.

### Templating and Shortcodes

Hugo content files (the markdown) cannot have templates executed within them. They _do_ have a limited form of templating called [shortcodes](https://gohugo.io/content-management/shortcodes/), and shortcodes have template code that is executed. These are executed when the markdown page is rendered into HTML. These have the syntax of:
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

This has the consequence that all template commands are processed _before_ the content file goes in, so any template instructions within the content file are ignored. Thus, your code is safe.

## Shortcodes, Layouts, and Other Library

Along with the default shortcodes, you also have access to some more added by the site or our theme, Hextra.

### Hextra

- Our theme's shortcodes can be found [in their docs](https://imfing.github.io/hextra/docs/guide/shortcodes/)
- Hextra also allows use of the following:
  - Syntax Highlighting
  - LaTeX
  - Diagrams/flow charts

Note that we prefer to use Github's note markdown syntax as opposed to callouts.

### Recoil Website

A few shortcodes and layouts have been written for the website specifically. They won't really be useful in most circumstances, but they are worth writing down.

- Shortcode `contributors` will render a list of contributors from Github. This is used for the site's homepage.
- Shortcode `latest_release` will render links for the latest release of Recoil, as well as download links and the releases page. This is also used for the site's homepage.
- Layout `commands` will render out a list of chat commands from a provided data file in the `data` directory of the site. This is used for the [Unsynced](content\docs\unsynced-commands.md) and [Synced](content\docs\synced-commands.md) pages. It expects a JSON file with the following schema:

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

[Hugo]: https://gohugo.io/
