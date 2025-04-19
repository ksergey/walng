[<img src="https://img.shields.io/github/license/ksergey/walng">](https://opensource.org/license/mit)
[<img src="https://img.shields.io/github/actions/workflow/status/ksergey/walng/build-and-test.yml?logo=linux">](https://github.com/ksergey/walng/actions/workflows/build-and-test.yml)
[<img src="https://img.shields.io/badge/language-C%2B%2B23-red">](https://en.wikipedia.org/wiki/C%2B%2B23)

# walng

Template colorscheme generator for base16 framework (https://github.com/chriskempson/base16)

I didn’t find a utility that suited my needs, so I wrote my own. Sorry for that.

# Building

You need c++ compiler and cmake. Everything else willbe downloaded.

```sh
git clone https://github.com/ksergey/walng.git && cd walng && mkdir build && cd build && cmake .. && make

```

# How to use?

Download base16 (or base24) scheme you like from https://github.com/tinted-theming/schemes/tree/spec-0.11/base16 and
run walng:

```sh
curl -O -L https://raw.githubusercontent.com/tinted-theming/schemes/refs/heads/spec-0.11/base16/terracotta.yaml && ./walng --theme terracotta.yaml

```

# Templates

Look up for templating examples in templates folder, they look more-less like this:

```jinja
* {
  background: rgba({{ rgb(palette.base00) }}, 0.8);

{% for name, color in palette %}
  {{ name }}: {{ color }};
{% endfor %}
}

```

Expression `{{ palette.colorNN }}` formats color as `#RRGGBB` (hex-digits with prefix `#`)
Expression `{{ rgb(palette.colorNN) }}` formats color as `R, G, B` (digits)
Expression `{{ hex_stripped(palette.colorNN) }}` formats color as `RRGGBB` (hex-digits)

# Configuration

walng configuration should be here `$XDG_CONFIG_HOME/walng/config.yaml`

Configuraion look like this:

```yaml
config:
  shell: "sh -c '{}'"

items:
  - name: "rofi"
    template: "~/.config/walng/templates/rofi-colors.rasi"
    target: "~/.config/rofi/colors.rasi"
    hook: ""

  - name: "waybar"
    template: "~/.config/walng/templates/waybar-colors.css"
    target: "~/.config/waybar/colors.css"
    hook: "killall -SIGUSR2 waybar"

```
