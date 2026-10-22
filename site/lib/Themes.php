<?php
declare(strict_types=1);

// Commodore 64 VIC-II colour indices, represented by approximate web RGB values.
function s64_theme_palette(): array
{
    return [
        '#000000' => 'Black', '#ffffff' => 'White',
        '#813338' => 'Red', '#75cec8' => 'Cyan',
        '#8e3c97' => 'Purple', '#56ac4d' => 'Green',
        '#2e2c9b' => 'Blue', '#edf171' => 'Yellow',
        '#8e5029' => 'Orange', '#553800' => 'Brown',
        '#c46c71' => 'Pink', '#4a4a4a' => 'Dark grey',
        '#7b7b7b' => 'Grey', '#a9ff9f' => 'Light green',
        '#706deb' => 'Light blue', '#b2b2b2' => 'Light grey',
    ];
}

// Legacy colours remain stored unchanged; rendering maps them to the nearest
// C64 shade without rewriting the user's saved theme or configuration.
function s64_theme_nearest_colour(string $hex): string
{
    $hex = strtolower($hex);
    $palette = s64_theme_palette();
    if (isset($palette[$hex])) return $hex;
    if (!preg_match('/^#[0-9a-f]{6}$/', $hex)) return '#ffffff';
    $rgb = [hexdec(substr($hex, 1, 2)), hexdec(substr($hex, 3, 2)), hexdec(substr($hex, 5, 2))];
    $best = '#ffffff';
    $score = PHP_INT_MAX;
    foreach ($palette as $colour => $label) {
        $distance = 0;
        foreach ([1, 3, 5] as $i => $offset) {
            $diff = $rgb[$i] - hexdec(substr($colour, $offset, 2));
            $distance += $diff * $diff;
        }
        if ($distance < $score) { $score = $distance; $best = $colour; }
    }
    return $best;
}

function s64_theme_palette_values(array $values): array
{
    foreach (s64_theme_fields() as $key => $field) {
        if ($field[0] === 'color' && isset($values[$key]) && is_string($values[$key]))
            $values[$key] = s64_theme_nearest_colour($values[$key]);
    }
    return $values;
}

// One schema drives validation and the ordinary HTML editor.
function s64_theme_fields(): array
{
    $fields = [];
    foreach (['text', 'background', 'link', 'visited', 'heading', 'border', 'panel', 'menu', 'menu_text', 'input', 'input_text', 'button', 'button_text'] as $name) {
        $fields[$name . '_color'] = ['color', ucwords(str_replace('_', ' ', $name)) . ' colour'];
    }
    foreach (['font_size' => [10, 32], 'content_width' => [20, 100], 'page_margin' => [0, 48], 'padding' => [0, 32], 'panel_padding' => [0, 32], 'paragraph_spacing' => [0, 32], 'letter_spacing' => [0, 4], 'border_width' => [0, 8], 'corner_radius' => [0, 24], 'menu_padding' => [0, 24], 'control_padding' => [0, 16]] as $name => $bounds) {
        $fields[$name] = ['number', ucwords(str_replace('_', ' ', $name)) . ($name === 'content_width' ? ' (em)' : ' (px)'), ...$bounds];
    }
    $fields['font'] = ['choice', 'Font', ['monospace', 'c64', 'sans', 'serif', 'arial', 'verdana', 'trebuchet', 'tahoma', 'georgia', 'times', 'courier']];
    $fields['line_height'] = ['choice', 'Line height', ['1', '1.25', '1.5', '1.75', '2']];
    $fields['border_style'] = ['choice', 'Border style', ['none', 'solid', 'double', 'inset', 'outset']];
    $fields['alignment'] = ['choice', 'Text alignment', ['left', 'center', 'right']];
    $fields['menu_layout'] = ['choice', 'Menu layout', ['inline', 'stacked']];
    $fields['font_weight'] = ['choice', 'Text weight', ['normal', 'bold']];
    $fields['header_title'] = ['text', 'Website title', 80];
    $fields['show_title'] = ['choice', 'Show website title', ['yes', 'no']];
    $fields['show_menu'] = ['choice', 'Show navigation menu', ['yes', 'no']];
    $fields['show_separator'] = ['choice', 'Show line below header', ['yes', 'no']];
    $fields['content_style'] = ['choice', 'Content layout', ['panel', 'open']];
    return $fields;
}

function s64_theme_field_groups(): array
{
    return [
        'Page layout' => ['header_title', 'show_title', 'show_menu', 'show_separator', 'content_style', 'content_width', 'page_margin', 'padding', 'panel_padding', 'alignment', 'menu_layout'],
        'Typography' => ['font', 'font_size', 'font_weight', 'line_height', 'letter_spacing', 'paragraph_spacing'],
        'Colours' => ['text_color', 'background_color', 'heading_color', 'link_color', 'visited_color', 'panel_color', 'border_color', 'menu_color', 'menu_text_color', 'input_color', 'input_text_color', 'button_color', 'button_text_color'],
        'Borders and controls' => ['border_style', 'border_width', 'corner_radius', 'menu_padding', 'control_padding'],
    ];
}

function s64_theme_defaults(array $config = []): array
{
    $values = [
        'text_color' => '#ffffff', 'background_color' => '#2e2c9b', 'link_color' => '#edf171', 'visited_color' => '#a9ff9f',
        'heading_color' => '#ffffff', 'border_color' => '#ffffff', 'panel_color' => '#2e2c9b',
        'menu_color' => '#2e2c9b', 'menu_text_color' => '#ffffff', 'input_color' => '#ffffff', 'input_text_color' => '#000000',
        'button_color' => '#ffffff', 'button_text_color' => '#000000', 'font' => 'monospace', 'font_size' => '16',
        'content_width' => '40', 'page_margin' => '16', 'padding' => '16', 'panel_padding' => '16',
        'paragraph_spacing' => '16', 'letter_spacing' => '0', 'border_width' => '1', 'corner_radius' => '0',
        'menu_padding' => '8', 'control_padding' => '4', 'line_height' => '1.5', 'border_style' => 'solid',
        'alignment' => 'left', 'menu_layout' => 'inline', 'font_weight' => 'normal',
        'header_title' => (string)($config['site_title'] ?? ''), 'show_title' => 'yes', 'show_menu' => 'yes',
        'show_separator' => 'yes', 'content_style' => 'panel',
    ];
    foreach (['text_color', 'background_color', 'link_color', 'visited_color'] as $key) {
        if (isset($config[$key]) && is_string($config[$key]) && preg_match('/^#[0-9a-f]{6}$/i', $config[$key])) {
            $values[$key] = s64_theme_nearest_colour($config[$key]);
        }
    }
    $values['panel_color'] = $values['menu_color'] = $values['background_color'];
    $values['menu_text_color'] = $values['link_color'];
    $values['heading_color'] = $values['border_color'] = $values['text_color'];
    return $values;
}

function s64_validate_theme(array $input, array $defaults = []): array
{
    $base = array_replace(s64_theme_defaults(), $defaults);
    $result = [];
    foreach (s64_theme_fields() as $key => $field) {
        $raw = array_key_exists($key, $input) ? $input[$key] : $base[$key];
        if (!is_scalar($raw)) throw new InvalidArgumentException('invalid_' . $key);
        $value = trim((string)$raw);
        $valid = match ($field[0]) {
            'color' => isset(s64_theme_palette()[strtolower($value)]),
            'number' => preg_match('/^[0-9]{1,3}$/', $value) === 1 && (int)$value >= $field[2] && (int)$value <= $field[3],
            'choice' => in_array($value, $field[2], true),
            'text' => strlen($value) <= $field[2],
        };
        if (!$valid) throw new InvalidArgumentException('invalid_' . $key);
        $result[$key] = $field[0] === 'number' ? (string)(int)$value : ($field[0] === 'color' || $field[0] === 'choice' ? strtolower($value) : $value);
    }
    foreach ([['text_color', 'background_color'], ['link_color', 'background_color'], ['visited_color', 'background_color'], ['text_color', 'panel_color'], ['link_color', 'panel_color'], ['visited_color', 'panel_color'], ['heading_color', 'panel_color'], ['heading_color', 'background_color'], ['menu_text_color', 'menu_color'], ['visited_color', 'menu_color'], ['input_text_color', 'input_color'], ['button_text_color', 'button_color']] as [$fg, $bg]) {
        if (s64_contrast_ratio($result[$fg], $result[$bg]) < 4.5) throw new InvalidArgumentException('insufficient_contrast');
    }
    return $result;
}

function s64_contrast_ratio(string $first, string $second): float
{
    $luminance = static function (string $color): float {
        $channels = [];
        foreach ([1, 3, 5] as $offset) {
            $v = hexdec(substr($color, $offset, 2)) / 255;
            $channels[] = $v <= 0.04045 ? $v / 12.92 : (($v + 0.055) / 1.055) ** 2.4;
        }
        return $channels[0] * 0.2126 + $channels[1] * 0.7152 + $channels[2] * 0.0722;
    };
    $a = $luminance($first); $b = $luminance($second);
    return (max($a, $b) + 0.05) / (min($a, $b) + 0.05);
}

function s64_custom_theme_css(array $input): string
{
    // Revalidate at rendering too: never interpolate raw configuration into CSS.
    try { $v = s64_validate_theme(s64_theme_palette_values($input)); } catch (InvalidArgumentException) { $v = s64_theme_defaults(); }
    $fonts = [
        'c64' => '"See64You C64","Courier New",Courier,monospace',
        'monospace' => '"Courier New",Courier,monospace',
        'sans' => 'Arial,Helvetica,sans-serif',
        'serif' => 'Georgia,"Times New Roman",serif',
        'arial' => 'Arial,Helvetica,sans-serif',
        'verdana' => 'Verdana,Geneva,sans-serif',
        'trebuchet' => '"Trebuchet MS",Arial,sans-serif',
        'tahoma' => 'Tahoma,Verdana,sans-serif',
        'georgia' => 'Georgia,"Times New Roman",serif',
        'times' => '"Times New Roman",Times,serif',
        'courier' => '"Courier New",Courier,monospace',
    ];
    $css = $v['font'] === 'c64' ? '@font-face{font-family:"See64You C64";src:url("assets/fonts/c64.ttf") format("truetype");font-weight:normal;font-style:normal;}' : '';
    $css .= 'body{margin:' . $v['page_margin'] . 'px auto;padding:' . $v['padding'] . 'px;max-width:' . $v['content_width'] . 'em;font-family:' . $fonts[$v['font']] . ';font-size:' . $v['font_size'] . 'px;line-height:' . $v['line_height'] . ';letter-spacing:' . $v['letter_spacing'] . 'px;font-weight:' . $v['font_weight'] . ';text-align:' . $v['alignment'] . ';color:' . $v['text_color'] . ';background:' . $v['background_color'] . ';}';
    $css .= 'a{color:' . $v['link_color'] . ';text-decoration:underline}a:visited{color:' . $v['visited_color'] . '}a:focus,input:focus,select:focus,button:focus{outline:2px solid ' . $v['heading_color'] . '}';
    $css .= '.s64-panel{background:' . ($v['content_style'] === 'open' ? 'transparent' : $v['panel_color']) . ';padding:' . ($v['content_style'] === 'open' ? '0' : $v['panel_padding'] . 'px') . ';border:' . ($v['content_style'] === 'open' ? '0' : $v['border_width'] . 'px ' . $v['border_style'] . ' ' . $v['border_color']) . ';border-radius:' . $v['corner_radius'] . 'px;}';
    $css .= 'h1,h2,h3{color:' . $v['heading_color'] . '}p{margin:0 0 ' . $v['paragraph_spacing'] . 'px}.s64-menu{background:' . $v['menu_color'] . ';padding:' . $v['menu_padding'] . 'px}.s64-menu a{color:' . $v['menu_text_color'] . ';display:' . ($v['menu_layout'] === 'stacked' ? 'block' : 'inline') . '}.s64-menu a:visited{color:' . $v['visited_color'] . '}' . ($v['menu_layout'] === 'stacked' ? '.s64-menu-separator{display:none}' : '');
    $css .= 'hr{border:0;border-top:' . $v['border_width'] . 'px ' . $v['border_style'] . ' ' . $v['border_color'] . '}input,select,textarea,button{font-family:' . $fonts[$v['font']] . ';font-size:' . $v['font_size'] . 'px;padding:' . $v['control_padding'] . 'px;max-width:100%;}input,select,textarea{color:' . $v['input_text_color'] . ';background:' . $v['input_color'] . '}button,input[type=submit]{color:' . $v['button_text_color'] . ';background:' . $v['button_color'] . '}pre{overflow:auto}';
    return $css;
}
