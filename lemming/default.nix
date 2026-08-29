{ mkPebbleWatchface, imagemagick }:

mkPebbleWatchface {
  src = ./.;

  # to_bw.sh needs magick, and nothing else.
  extraNativeBuildInputs = [ imagemagick ];

  # package.json points diorite and flint at lemming_bank_bw_144x168.png and
  # six lemmings_walk_bw_f*_144x168.png frames. Those are committed, and so is
  # the colour walk GIF to_bw.sh derives them from — that last part matters: a
  # flake src contains only tracked files, and this step once broke the tagged
  # build because the GIF lived under the ignored drafts/ dir. Regenerating
  # here keeps the 1-bit assets honest: to_bw.sh is a pure per-pixel
  # substitution over the colour art, so the build ships assets derived from
  # the committed sources even if the committed outputs have drifted.
  preBuild = "bash resources/scripts/to_bw.sh";
}
