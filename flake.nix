{
  description = "Pebble development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nixpkgs-python.url = "github:cachix/nixpkgs-python";
    systems.url = "github:nix-systems/default";
    devenv.url = "github:cachix/devenv";
  };

  outputs = { self, nixpkgs, devenv, systems, ... } @ inputs:
    let
      forEachSystem = nixpkgs.lib.genAttrs [ "x86_64-linux" "x86_64-darwin" "aarch64-darwin" ];
    in
    {
      packages = forEachSystem (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          mkPebbleWatchface = pkgs.callPackage ./nix/pebble-watchface.nix { };
        in
        {
          devenv-up = self.devShells.${system}.default.config.procfileScript;

          # All three need `--option sandbox false`: the builds pip-install
          # pebble-tool and download the SDK.
          lemming = pkgs.callPackage ./lemming { inherit mkPebbleWatchface; };
          meow-o-clock = pkgs.callPackage ./meow-o-clock { inherit mkPebbleWatchface; };
          moonphase = pkgs.callPackage ./moonphase { inherit mkPebbleWatchface; };
        });

      devShells = forEachSystem (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = devenv.lib.mkShell {
            inherit inputs pkgs;
            modules = [
              {
                languages.c.enable = true;
                languages.python = {
                  enable = true;
                  version = "3.12";
                  venv.enable = true;
                  venv.requirements = ''
                    pebble-tool==5.0.40
                  '';
                };

                packages = with pkgs; [
                  clang-tools
                  imagemagick
                  ffmpeg
                  # SDK 4.33.1's qemu-pebble links against libpng16 and ships no
                  # copy of it, so the emulator will not launch without this on
                  # the dyld search path. 4.9.169's qemu did not need it.
                  libpng
                  # lemming asset generation: curl talks to OpenRouter, jq builds
                  # the request and pulls the base64 image out of the response.
                  curl
                  jq
                ];

                scripts = {
                  lemming-generate = {
                    description = "Generate the lemming character sprite via OpenRouter and emit Pebble assets";
                    exec = ''
                      exec "$DEVENV_ROOT/lemming/resources/scripts/generate_v4.sh" "$@"
                    '';
                  };
                  lemming-convert = {
                    description = "Convert an existing sprite PNG into 200x228 + 144x168 RGB222 Pebble assets";
                    exec = ''
                      exec "$DEVENV_ROOT/lemming/resources/scripts/to_pebble.sh" "$@"
                    '';
                  };
                  lemming-bank = {
                    description = "Generate the Lemming Brothers bank background and emit Pebble assets";
                    exec = ''
                      set -e
                      root="$DEVENV_ROOT/lemming"
                      "$root/resources/scripts/generate_image.sh" \
                        "$root/resources/prompts/lemming_bank.md" lemming_bank
                      "$root/resources/scripts/scene_to_pebble.sh" \
                        "$root/resources/images/lemming_bank_raw.png" lemming_bank \
                        --crop-w 813 --cut 560,177 --sky 000055
                    '';
                  };
                  lemming-walk = {
                    description = "Generate the three-lemming walk cycle (image -> veo video -> sprite strip)";
                    exec = ''
                      set -e
                      root="$DEVENV_ROOT/lemming"
                      "$root/resources/scripts/generate_image.sh" \
                        "$root/resources/prompts/lemming_bank_walk.md" lemming_bank_walk
                      "$root/resources/scripts/generate_video.sh" \
                        "$root/resources/prompts/lemming_bank_walk.md" lemming_bank_walk 4
                      "$root/resources/scripts/walk_to_pebble.sh" \
                        "$root/resources/video/lemming_bank_walk.mp4" lemmings_walk
                    '';
                  };
                  lemming-bw = {
                    description = "Derive the 1-bit diorite/flint assets from the composed colour ones";
                    exec = ''
                      exec "$DEVENV_ROOT/lemming/resources/scripts/to_bw.sh" "$@"
                    '';
                  };
                  lemming-preview = {
                    description = "Render the assembled Lemming Brothers watchface as an animated GIF";
                    exec = ''
                      exec "$DEVENV_ROOT/lemming/resources/scripts/preview_watchface.sh" "$@"
                    '';
                  };
                };

                git-hooks.hooks = {
                  clang-format = {
                    enable = true;
                    types_or = [ "c" "c++" ];
                  };
                  pebble-build = {
                    enable = false;
                    name = "pebble build";
                    entry = "pebble build";
                    files = "\\.c$";
                    pass_filenames = false;
                    stages = [ "pre-commit" ];
                  };
                };

                enterShell = ''
                  echo "Pebble development environment loaded"
                  echo "Available tools: pebble, clang-format, magick, ffmpeg"
                  echo "Lemming assets: lemming-generate [name], lemming-convert <png> <name>"
                  echo "Lemming Brothers: lemming-bank, lemming-walk, lemming-bw, lemming-preview [HH:MM]"

                  # devenv exports CC=clang, which hijacks waf's ARM
                  # cross-compiler detection, and pebble-tool's qemu version
                  # check needs PEBBLE_QEMU_PATH and DYLD_LIBRARY_PATH to find
                  # its bundled qemu on macOS. Scope the fixes to pebble alone
                  # so DYLD_LIBRARY_PATH doesn't shadow libraries for other
                  # tools in the shell.
                  pebble() {
                    local sdk="$HOME/Library/Application Support/Pebble SDK/SDKs/current"
                    if [ -x "$sdk/toolchain/bin/qemu-pebble" ]; then
                      env -u CC -u CXX \
                        PEBBLE_QEMU_PATH="$sdk/toolchain/bin/qemu-pebble" \
                        DYLD_LIBRARY_PATH="$sdk/toolchain/lib" \
                        pebble "$@"
                    else
                      env -u CC -u CXX pebble "$@"
                    fi
                  }
                '';
              }
            ];
          };
        });
    };
}
