{
  description = "Vulkan C application with GLFW";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "vulkan-app";
          version = "0.1.0";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
            shaderc
          ];

          buildInputs = with pkgs; [
            glfw
            vulkan-headers
            vulkan-loader
            vulkan-validation-layers
            xorg.libX11
            xorg.libXrandr
            xorg.libXinerama
            xorg.libXcursor
            xorg.libXi
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Debug"
          ];

          installPhase = ''
            mkdir -p $out/bin
            cp app $out/bin/
            
            # Copy compiled shaders if they exist
            if [ -d "shaders" ]; then
              mkdir -p $out/share/shaders
              cp -r shaders/* $out/share/shaders/
            fi
          '';
        };

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            cmake
            pkg-config
            glfw
            vulkan-headers
            vulkan-loader
            vulkan-validation-layers
            vulkan-tools
            shaderc
            
            xorg.libX11
            xorg.libXrandr
            xorg.libXinerama
            xorg.libXcursor
            xorg.libXi
            
            # LSP and development tools
            clang-tools
            
            # Formatters
            nixpkgs-fmt
            stylua
            nodePackages.prettier
          ];

          shellHook = ''
            echo "🔥 Vulkan development environment loaded"
            echo "CMake: $(cmake --version | head -n1)"
            echo "Vulkan SDK: $(vulkaninfo --summary 2>/dev/null | grep 'Vulkan Instance Version' || echo 'installed')"
            echo "Clangd: $(clangd --version | head -n1)"
            echo ""
            echo "🔨 Build with: cd build && cmake .. && make"
            echo "📄 compile_commands.json will be auto-generated"
            echo "🐛 Debug with: <F5> in nvim (codelldb)"
            echo ""
            
            # Auto-generate compile_commands.json
            if [ ! -d "build" ]; then
              echo "📂 Creating build directory and generating compile_commands.json..."
              mkdir -p build
              cd build
              cmake .. > /dev/null 2>&1
              cd ..
              echo "✅ compile_commands.json generated!"
            fi
            
            # Enable validation layers for debugging
            export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
          '';

          # Environment variables
          PKG_CONFIG_PATH = "${pkgs.glfw}/lib/pkgconfig:${pkgs.vulkan-loader}/lib/pkgconfig";
          CPATH = "${pkgs.glfw}/include:${pkgs.vulkan-headers}/include";
          VK_LAYER_PATH = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
          LD_LIBRARY_PATH = "${pkgs.vulkan-loader}/lib:${pkgs.vulkan-validation-layers}/lib";
        };
      }
    );
}
