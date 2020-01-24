#ifndef PAGE_STYLE_CSS_H
#define PAGE_STYLE_CSS_H

const char PAGE_Style_css[] PROGMEM = R"=====(
hr {
    background-color: #eee;
    border: 0 none;
    color: #eee;
    height: 1px;
}
.btn, .btn:link, .btn:visited {
    border-radius: 0.3em;
    border-style: solid;
    border-width: 1px;
    color: #111;
    display: inline-block;
    font-family: avenir, helvetica, arial, sans-serif;
    letter-spacing: 0.15em;
    margin-bottom: 0.5em;
    padding: 1em 0.75em;
    text-decoration: none;
    text-transform: uppercase;
    -webkit-transition: color 0.4s, background-color 0.4s, border 0.4s;
    transition: color 0.4s, background-color 0.4s, border 0.4s;
}
.btn:hover, .btn:focus {
    color: #DFE0DF;
    border: 1px solid #DFE0DF;
    -webkit-transition: background-color 0.3s, color 0.3s, border 0.3s;
    transition: background-color 0.3s, color 0.3s, border 0.3s;
}
.btn:active {
    color: #0094FF;
    border: 1px solid #0094FF;
    -webkit-transition: background-color 0.3s, color 0.3s, border 0.3s;
    transition: background-color 0.3s, color 0.3s, border 0.3s;
}
.btn--s{
    font-size: 12px;
}
.btn--m {
    font-size: 14px;
}
.btn--l {
    font-size: 20px;
    border-radius: 0.25em !important;
}
.btn--full, .btn--full:link {
    border-radius: 0.25em;
    display: block;
    margin-left: auto;
    margin-right: auto;
    text-align: center;
    width: 100%;
}
.btn--blue:link, .btn--blue:visited {
    color: #fff;
    background-color: #0094FF;
}
.btn--blue:hover, .btn--blue:focus {
    color: #fff !important;
    background-color: #0026FF;
    border-color: #0026FF;
}
.btn--blue:active {
    color: #fff;
    background-color: #20007A;
    border-color: #20007A;
}
@media screen and (min-width: 32em) {
    .btn--full {
        max-width: 16em !important;
    }
}
)=====";

#endif
