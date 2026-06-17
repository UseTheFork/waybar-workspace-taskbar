{
  inputs = {
    systems.url = "github:nix-systems/default-linux";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    inputs@{
      flake-parts,
      nixpkgs,
      ...
    }:
    flake-parts.lib.mkFlake
      {
        inherit inputs;
      }
      (
        {
          withSystem,
          flake-parts-lib,
          inputs,
          self,
          ...
        }:
        {
          systems = import inputs.systems;
          perSystem = { pkgs, inputs', ... }: {
            packages.default = pkgs.stdenv.mkDerivation {
              name = "waybar-workspace-taskbar";

              src = pkgs.lib.cleanSource ./.;

              nativeBuildInputs = [
                pkgs.pkg-config
                pkgs.meson
                pkgs.ninja
              ];

              buildInputs = [
                pkgs.gtk3
                pkgs.glib
                pkgs.json-glib
              ];

              installPhase = ''
                runHook preInstall
                mkdir -p $out
                cp waybar-workspace-taskbar.so $out/
                runHook postInstall
              '';

              meta = {
                description = "Workspace taskbar for Waybar showing only open windows from the current workspace";
                homepage = "https://github.com/stevekanger/waybar-workspace-taskbar";
                license = pkgs.lib.licenses.mit;
                platforms = pkgs.lib.platforms.linux;
              };
            };

            devShells.default = pkgs.mkShellNoCC {
              name = "nix";

              # Tell Direnv to shut up.
              DIRENV_LOG_FORMAT = "";

              packages = [
                # Packages from nixpkgs, for Nix, Flakes or local tools.
                pkgs.just # Command Runner

                # Tools / Formaters Linters etc
                pkgs.alejandra # Nix
                pkgs.keep-sorted # General Sorting tool

              ];
            };
          };
        }
      );
}
