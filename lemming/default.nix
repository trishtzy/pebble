{ mkPebbleWatchface }:

mkPebbleWatchface {
  src = ./.;

  # No preBuild. Everything package.json names — including the diorite/flint
  # lemming_bank_bw_144x168.png and the six lemmings_walk_bw_f*_144x168.png
  # frames — is committed; resources/images holds exactly those plus the two
  # bank previews, and .gitignore ignores only the drafts/ subdirectory of
  # generation intermediates beneath it.
  #
  # This used to run `to_bw.sh` here to regenerate the 1-bit assets, on the
  # premise that they were untracked while their sources were tracked. Both
  # halves are false: the outputs ARE tracked, and the script's walk-cycle
  # input (drafts/lemmings_walk_144x168.gif) is NOT. A flake src sees only
  # tracked files, so the step worked from a dirty checkout and died in CI —
  # see the failed lemming/v1.0.0 release. Do not reinstate it without tracking
  # that gif first, and there is little to gain by doing so: run against the
  # committed colour art, to_bw.sh reproduces the committed 1-bit assets pixel
  # for pixel.
}
