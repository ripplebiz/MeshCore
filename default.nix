{ pkgs ? import <nixpkgs> {} }:
let
in
  pkgs.mkShell {
    buildInputs = [
      pkgs.platformio
      # optional: needed as a programmer i.e. for esp32
      pkgs.avrdude
    ];
}
