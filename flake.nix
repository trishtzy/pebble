{
  description = "Pebble development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nixpkgs-python.url = "github:cachix/nixpkgs-python";
    systems.url = "github:nix-systems/default";
    devenv.url = "github:cachix/devenv";
  };

  nixConfig = {
    extra-trusted-public-keys = "devenv.cachix.org-1:w1cLUi8dv3hnoSPGAuibQv+f9TZLr6cv/Hm9XgU50cw=";
    extra-substituters = "https://devenv.cachix.org";
  };

  outputs = { self, nixpkgs, devenv, systems, ... } @ inputs:
    let
      forEachSystem = nixpkgs.lib.genAttrs [ "x86_64-darwin" "aarch64-darwin" ];
    in
    {
      packages = forEachSystem (system: {
        devenv-up = self.devShells.${system}.default.config.procfileScript;
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
                    pebble-tool==5.0.16
                  '';
                };

                packages = with pkgs; [
                  clang-tools
                  imagemagick
                  ffmpeg
                ];

                pre-commit.hooks = {
                  clang-format = {
                    enable = true;
                    types_or = [ "c" "c++" ];
                  };
                  pebble-build = {
                    enable = true;
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
                '';
              }
            ];
          };
        });
    };
}
