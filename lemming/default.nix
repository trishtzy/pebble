{ mkPebbleWatchface, imagemagick }:

mkPebbleWatchface {
  src = ./.;

  # to_bw.sh needs magick, and nothing else.
  extraNativeBuildInputs = [ imagemagick ];

  # package.json points diorite and flint at lemming_bank_bw_144x168.png and
  # six lemmings_walk_bw_f*_144x168.png frames, and emery's B/W setting at
  # lemming_bank_bw_200x228.png and the lemmings_walk_bw_200x228.png APNG.
  # Those are committed, and so are the colour walk GIFs to_bw.sh derives them
  # from — that last part matters: a flake src contains only tracked files, and
  # this step once broke the tagged build because a GIF lived under the ignored
  # drafts/ dir. Regenerating here keeps the derived assets honest: to_bw.sh is
  # a pure per-pixel substitution over the colour art, so the build ships
  # assets derived from the committed sources even if the committed outputs
  # have drifted.
  preBuild = "bash resources/scripts/to_bw.sh";
}
