# Gilded length and Master blade-root adjustment

The user approved the final Master and Gilded models shown in preview commit
`48308c553619265a548ab7d2dfa142ef43232e83`. True Master was accepted before
this adjustment. These versions are the approved visual baseline; approval
does not replace in-game verification.

The subsequent [Master hilt symmetry review](master-hilt-review.md) checks
the perceived asymmetry without changing the approved model.

This pass follows the latest feedback: Gilded was too long, Master's narrow
blade section above the guard was too thin, and True Master was accepted.

- [Two before/after pairs at one common scale](swords_tuned_before_after_scale.png)
- [Gilded, Master and accepted True Master rotating](swords_tuned_group_2.gif)
- [All nine swords rotating](swords_tuned.gif)
- [All models guard-aligned, effects off](swords_tuned_measured_scale.png)

Gilded's visible blade is about 21% shorter. Its guard, red grip, silver pommel,
blade width and three broad gold diamonds remain. The pattern compresses along
the blade, rather than stretching a uniformly scaled sword.

Master's blade root is widened from about 42% to 72% of the main blade width.
The stepped shoulders, long main blade, guard and grip retain their dimensions.

| Sword | Previous full length | Current full length |
|---|---:|---:|
| Gilded | 101.6 | 83.8 |
| Master | 92.4 | 92.4 |
| True Master, accepted | 92.4 | 92.4 |

Lengths are exported model units before host actor transforms. True Master's
source-generated mesh and installed resources are preserved exactly. The other
six swords also retain their meshes and resources. These comparisons use one
camera scale and align guards using translation only.

Gilded's golden motes follow the shortened blade. Shimmer colors and the other
particle themes remain approved. Lighting and reflections are offline
approximations; in-game appearance remains unverified. Reference images guide
the shapes but do not supply recovered Nintendo GI dimensions.

The approved implementation, assets, rebuild tools and these review notes are
included together on the `slate-element-preview` development branch.

## Verification

- All 60 serialized GI models pass geometry/winding, vertex-cache, references,
  matrix, texture and transparency checks.
- Focused geometry checks confirm the wider Master root, shorter Gilded and
  retained broad diamond coverage. Hash checks preserve all seven other swords,
  including the accepted True Master.
- Fresh combo diagnostics pass native and foreign draws, local and foreign
  shelf poses, optional shimmer, effect attachment and resource fallbacks.
- The production effect policy passes AddressSanitizer and
  UndefinedBehaviorSanitizer; Gilded motes reach its upper blade without
  extending to the old tip.
- All four GIFs contain 72 distinct looping frames. The same camera scale
  contains all current and comparison models and their effects without clipping.
- Independent source, export and preview review found no critical or important
  issues. In-game viewing is still needed to assess the final appearance.
