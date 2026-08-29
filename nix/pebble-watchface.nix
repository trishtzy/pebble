# Shared builder for the Pebble watchfaces in this repo.
#
#   mkPebbleWatchface { src = ./.; preBuild = "..."; }
#
# Every project here builds the same way — venv, pebble-tool, SDK, `pebble
# build` — but four of the steps below are workarounds for non-obvious
# breakage, and having one copy of them beats three that drift apart. The
# packaging this replaces drifted exactly that way: it pinned meow-o-clock at
# 1.3.0 against a 1.4.0 package.json, and copied a `Perryverse.pbw` that is
# really `watchface.pbw`.
#
# NOTE: the build needs network access (pip, and the SDK download), so it must
# run with the sandbox relaxed:
#
#   nix build .#lemming --option sandbox false
{ stdenv, lib, python312, cacert, nodejs, darwin }:

{ src
  # Shell run after the SDK is ready and before `pebble build`, from the
  # project root. For generated resources that are deliberately not tracked.
, preBuild ? ""
, extraNativeBuildInputs ? [ ]
, pebbleToolVersion ? "5.0.40"
}:

let
  # Single source of truth: both come from the project's own package.json, so
  # a version bump cannot leave the packaging behind.
  projectJson = lib.importJSON (src + "/package.json");
in
stdenv.mkDerivation rec {
  pname = projectJson.name;
  version = projectJson.version;

  inherit src;

  nativeBuildInputs = [
    python312
    cacert
    nodejs
  ] ++ lib.optionals stdenv.hostPlatform.isDarwin [
    darwin.cctools
  ] ++ extraNativeBuildInputs;

  # There is no ./configure; without this the generic builder wastes a phase
  # looking for one.
  dontConfigure = true;

  buildPhase = ''
    runHook preBuild

    export HOME=$TMPDIR
    export SSL_CERT_FILE=${cacert}/etc/ssl/certs/ca-bundle.crt

    python -m venv .venv
    source .venv/bin/activate
    pip install pebble-tool==${pebbleToolVersion}

    pebble sdk install latest

    # (1) pebble-tool hardcodes a symlink at /var/tmp/pebble-sdk to shorten the
    # SDK path for the ARM toolchain (the real one contains spaces). It unlinks
    # and recreates that path unconditionally, which fails in any sandboxed or
    # multi-user build where it already exists owned by someone else — a local
    # SDK install leaves it owned by root, and then every build on that host
    # dies with EACCES before compiling a thing. Relocate the link into this
    # build's temp dir; still no spaces, so it serves the same purpose.
    substituteInPlace .venv/lib/python3.12/site-packages/pebble_tool/sdk/__init__.py \
      --replace-fail 'tmp_link = "/var/tmp/pebble-sdk"' \
                     'tmp_link = os.environ.get("PEBBLE_SDK_LINK", "/var/tmp/pebble-sdk")'
    export PEBBLE_SDK_LINK="$TMPDIR/pebble-sdk"

    # (2) Put the bundled ARM cross-compiler on PATH so waf finds
    # arm-none-eabi-gcc; it searches PATH for 'gcc'/'cc'.
    export PATH="$HOME/Library/Application Support/Pebble SDK/SDKs/current/toolchain/arm-none-eabi/bin:$PATH"

    # (3) ...and PATH alone is not enough: stdenv exports CC=clang and waf
    # honours CC over its PATH search, so it "finds" the host clang and stops
    # with "Could not find gcc/g++ (only Clang)". Same collision the devShell's
    # pebble() wrapper works around with `env -u CC -u CXX`.
    unset CC CXX

    ${preBuild}

    # (4) build/*.pbw is tracked in git, so a stale bundle is present in the
    # source tree. Remove it first — otherwise a build that produced nothing
    # would still install, and ship the committed artifact as if it were fresh.
    rm -f build/*.pbw

    pebble build

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p $out

    # Located by glob rather than by name: the bundle is named for the project
    # DIRECTORY, not for package.json's `name` (lemming/ builds lemming.pbw
    # though the app is lemming-brothers), and hardcoding that is what broke
    # the previous packaging.
    pbws=(build/*.pbw)
    if [ ''${#pbws[@]} -ne 1 ]; then
      echo "expected exactly one .pbw in build/, found: ''${pbws[*]}" >&2
      exit 1
    fi
    cp "''${pbws[0]}" "$out/${pname}-v${version}.pbw"

    runHook postInstall
  '';

  meta = {
    description = "${projectJson.pebble.displayName or pname} Pebble watchface";
    platforms = lib.platforms.unix;
  };
}
