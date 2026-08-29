{ stdenv, lib, python312, cacert, nodejs, imagemagick, darwin }:

stdenv.mkDerivation rec {
  pname = "lemming-brothers";

  # Read from package.json rather than repeated here. The two packagings this
  # replaces both drifted out of sync with theirs (1.3.0 against a 1.4.0
  # package.json), which is a silently wrong release artifact rather than a
  # build failure — nothing checks the two agree.
  version = (lib.importJSON ./package.json).version;

  src = ./.;

  nativeBuildInputs = [
    python312
    cacert
    nodejs
    # For to_bw.sh below. Only `magick` is needed — the 1-bit assets are pure
    # colour substitution over art that is already committed.
    imagemagick
  ] ++ lib.optionals stdenv.hostPlatform.isDarwin [
    darwin.cctools
  ];

  buildPhase = ''
    export HOME=$TMPDIR
    export SSL_CERT_FILE=${cacert}/etc/ssl/certs/ca-bundle.crt

    python -m venv .venv
    source .venv/bin/activate
    pip install pebble-tool==5.0.40

    pebble sdk install latest

    # pebble-tool hardcodes a symlink at /var/tmp/pebble-sdk to shorten the SDK
    # path for the ARM toolchain (the real one contains spaces). It unlinks and
    # recreates that path unconditionally, which fails in any sandboxed or
    # multi-user build where the path already exists owned by someone else —
    # a local SDK install leaves it owned by root, and then every nix build on
    # that host dies with EACCES before compiling a thing. Relocate the link
    # into the build's own temp dir; it still has no spaces, so it serves the
    # same path-shortening purpose.
    substituteInPlace .venv/lib/python3.12/site-packages/pebble_tool/sdk/__init__.py \
      --replace-fail 'tmp_link = "/var/tmp/pebble-sdk"' \
                     'tmp_link = os.environ.get("PEBBLE_SDK_LINK", "/var/tmp/pebble-sdk")'
    export PEBBLE_SDK_LINK="$TMPDIR/pebble-sdk"

    # Add bundled ARM cross-compiler to PATH so WAF finds arm-none-eabi-gcc
    # (WAF searches PATH for 'gcc'/'cc'; without this it falls back to system clang)
    export PATH="$HOME/Library/Application Support/Pebble SDK/SDKs/current/toolchain/arm-none-eabi/bin:$PATH"

    # ...and PATH alone is not enough, because stdenv exports CC=clang and waf
    # honours CC over its PATH search: it then "finds" the host clang and stops
    # with "Could not find gcc/g++ (only Clang)". This is the same collision the
    # devShell's pebble() wrapper works around with `env -u CC -u CXX`.
    unset CC CXX

    # package.json points diorite and flint at lemming_bank_bw_144x168.png and
    # six lemmings_walk_bw_f*_144x168.png frames. Those are GENERATED and
    # deliberately untracked, so a checkout does not contain them and the
    # resource step for those two platforms would fail to resolve. Their
    # sources — the composed colour background and walk GIF — are tracked, and
    # to_bw.sh is a pure per-pixel substitution over them, so regenerating here
    # is reproducible and keeps derived art out of git.
    bash resources/scripts/to_bw.sh

    pebble build
  '';

  installPhase = ''
    mkdir -p $out
    # The bundle is named for the project DIRECTORY (lemming), not for
    # package.json's `name` (lemming-brothers). Getting this wrong is what left
    # the old watchface packaging copying a Perryverse.pbw that is really
    # watchface.pbw.
    cp build/lemming.pbw $out/${pname}-v${version}.pbw
  '';

  meta = {
    description = "Lemming Brothers Pebble watchface (basalt, diorite, flint, emery)";
    platforms = lib.platforms.unix;
  };
}
