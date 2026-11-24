# Emerald

## Theme

The light theme is defined entirely in `libs/emrldgui/res/style/*.css`. Edit those files and the app picks them up 1:1 — there is no theme selection in C++. This is the only theme shipped for now.

`Window.css` controls the window backdrop as well:

```css
Window {
    background-gradient: #ffffff, #f6f5f4;
    background-gradient-direction: vertical;

    /* optional, up to glow-3 */
    glow-1-color: rgba(34, 211, 238, 26);
    glow-1-x: 260px;
    glow-1-y: 120px;
    glow-1-radius: 660px;
}
```

ComboBox popup colors are CSS-only too: `popup-background-color`, `popup-border-color`, `item-hover-color` (hover background), `item-hover-text-color`, `item-selected-color` (selected background) and `item-selected-text-color`. The popup is rendered in a second pass, so it always draws above the other widgets even when they were added later.
