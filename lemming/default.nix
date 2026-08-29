{ mkPebbleWatchface, imagemagick }:

mkPebbleWatchface {
  src = ./.;

  # to_bw.sh needs magick, and nothing else.
  extraNativeBuildInputs = [ imagemagick ];

  # package.json points diorite and flint at lemming_bank_bw_144x168.png and
  # six lemmings_walk_bw_f*_144x168.png frames. Those are GENERATED and
  # deliberately untracked (see lemming/.gitignore), so a checkout does not
  # contain them and the resource step for those two platforms cannot resolve.
  # Their sources — the composed colour background and walk GIF — ARE tracked,
  # and to_bw.sh is a pure per-pixel substitution over them, so regenerating
  # here is reproducible and keeps derived art out of git.
  preBuild = "bash resources/scripts/to_bw.sh";
}
