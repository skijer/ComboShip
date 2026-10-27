"""Independent native baseline and optional visual layer preservation checks.

Fixed digests below were derived from the unchanged native game and accepted
visual input, independently of the new bundle builder. These tests exercise
prepared resources, not the builder's claimed preservation metadata.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import sys
import unittest
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
import resource_geometry as geometry

NATIVE_TEXTURE_HASHES = {'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_001618': '6eef04e5beb3976ffd08fc40ff920254b2d3d8bd082bbe938facee2dc4898cb0',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_002618': '82981efafe5da5a6db9bd0efa5dbaf9ac20a0ee48eaca7223d7bc2579b5fa0f5',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_003618': '5932c8851874a758581f5eaacf97db0ac051a1b9e45cabc81a039fe8092ce361',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_004618': '5dc464f2f17a17b1ed4e2de49df3fd9e9045c59f5767300c5f4a96118ec2b83b',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_005618': '3e2ca9f394c0f301c7cef9d6107cc6beec17e8b931cf9ec0a874dd81100c4a38',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_006618': '20f21bb7cb603d20c44ef5854f7c5ceb15b3edc62aea08e9f5cf047967b873ad',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_007618': '1bc5f2ac1ba6572a96d6df6b86576265a002ed4b8130fa8d288e58a5270b48f1',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_008618': '32bda032bb2bb4eb261bd74542b6d0760957d8bab445f68395d2ad686a3a0780',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_009618': '72011ed8a222c8001a6a57cef1d0fb5f3c317306e995f6c6d484ed8ecdccfdf0',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_00A618': '3d02e5473ceb70ada36e8de7d7e888c9ba85b52d1490035fd9fe2cdd4820735f',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_00B618': '7c9ee67651b799e9ebebc3044d50ae70bb9518daef140a8b367dec80aeb4d4f7',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_00C618': 'a4033ac6af90ba036fd4e961c71b6e4645dc7bc5ff3a4d0c39fca94f42b965d3',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_00D618': '56afca89377c30921d19c287f3a7b8e2c0792033fab31596eef49763a81eddfa',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_010CE8': '645e7c15761184f8e593e8cb7eaa5c6e695daf23b70430c7feb3b623b171bf1f',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_0114E8': '086fd3e6e03ea1593dc60d973e2de87cbad66ed6edf9fb029047059bba763581',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_011CE8': '250e959a708c0044383a0a313c238544de8b13570ffb5bb054429b7d9f354bb2',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_011D08': 'c5ff93dae18622fee4153bf2af85656eaa127e648fc8f0170fd2f1a7fb7e0b20',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_012508': '6a9e6f1158137176215aa40872998f938501dd2ca941f7e2624ea9b698bae660',
 'scenes/shared/kenjyanoma_scene/kenjyanoma_room_0Tex_012D08': 'da8232126ae394b99a7d0dbcc112e218959a82eb8ee9408a02c8282a30589b43',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0081D8': '28e123b3a39a09a930ddbc7a9ef9c988dc2d5ce767cce75845e8e68a68ce3c9f',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0091D8': '9662de612af8126b9c058ba80c8be51b88ea757ce71b89fb2c889774f4bbe4b6',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_00A1D8': '58be444d2cb0005a6537c76abba6ef8074d3a73aa5727142a5ed4ec316a2b36d',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_00B1D8': 'd70254b6ca075f73c9331fa3d91347e3f62e88a2f45137abbaa9f32ef2a12ea5',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_00C1D8': '88fd3203af449a955f60493054715e5f8df8925f66240afad9b4abe053483600',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_00D1D8': '5b2a6dcac5760c7820bb7b7d9d449ad3eaafbbb51e4dbffbfa745c01a5b47f9f',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_00E1D8': '13c9b7edc69a56bb031db62b7122bc27bfa4a63fc54840b77eef12f94b001f07',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_00F1D8': '0e80c1f41127b352a7d3ae46e2d6d71060bd78b411545eda72640a745023b100',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0101D8': '826a4403252e2f8cc762743e8ad751d5eea07727b2c667c7eda3c03cfa5c6423',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0111D8': 'a1c15a8563b12eff63608e25125539c510825f5922da4b4740d1607430a01d64',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0121D8': '5562ce97a08be1ad4fc583205c2cde95b84cc3d630ecbe2e7c770b34b02e7c1e',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0131D8': 'c5bb1d98ce8fdc3a8d2e7fd7c41dd98190456fcc7d1c40ca918dd0c6302a8444',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0141D8': '0f028ccdf60fbf30f99478b24472bca646c848f321d6562ab46a159ea1528dba',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0149D8': '4a1cdd3d6adb811d1d8a01a8bba6a6a939436f1e06c77a61898eb97c40b9eb6b',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0151D8': '6e5d9aac6b46f740b9bc3f5d94eeb8283b66a2f70f37da371b3bdb7d702267b6',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0159D8': 'a62a389bdd2f7ba096525b82e1986c34082ddaa09fab9c40b76ee9ea787fa299',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0161D8': '4dbc443df56e2770fba92e0b918eacf5b4dc3dadd3b76c06bca5605827cd5f0c',
 'scenes/shared/tokinoma_scene/tokinoma_room_0Tex_0169D8': 'c7674e756fe92414efe1db87a53909a99150549f983a141a49d05c75335ad685',
 'scenes/shared/tokinoma_scene/tokinoma_room_1Tex_005458': '4c56eb2a5297e8a5591c9045dc80386cde6a6e4b1ca19ab0c39c85afef3fa35d',
 'scenes/shared/tokinoma_scene/tokinoma_room_1Tex_005658': '0445b9c3de08887019d1b794c9c909ddd0a7c2b6e51630e96c17867af6528eb9',
 'scenes/shared/tokinoma_scene/tokinoma_room_1Tex_005858': '70dd880fd677f9361ffeb512f57eaa7d58b172ff7d9cf382e1de4ff16d33473c',
 'scenes/shared/tokinoma_scene/tokinoma_room_1Tex_006488': 'c838e0aa67118f52779c73b70d0a42a9e386dc8d05d955c0a3033dd7b83e590e',
 'scenes/shared/tokinoma_scene/tokinoma_room_1Tex_0064C8': '6e3656a0511de1947192d1d152cc9a4a5cc68cee9471bb4089f565e21e10e950',
 'scenes/shared/tokinoma_scene/tokinoma_room_1Tex_0068C8': 'ef0791fa7c2e3e99450177c76265f385b433cef5939d3fab4cd822055498c086',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_00CFA0': 'e6496304aecad96585904ed907b4285c6a9a7c032ab89484d9baaca8054baf1b',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_00D7A0': 'ccdcd6cc792ccec289aa51ac7dd97d3435a7712e630bb775c20c44fd9beadb06',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_00DFA0': '669749cd8a102266ed85f769aae49283cdea59c6e0c3bcaef8dfbf67261d9e35',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_00EFA0': '2113be07cdbcf9428df86bec5e9d0508dd65e65d7587ab1321f665ff7bfe360a',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_00FFA0': '327e1e5a681c859015861616af8e5ee587fb2456c9b9d21608cea857afc82df4',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_0107A0': 'a2e6bae77251050c34416b60389e3160d2f5445805d0344c32ad25e26182019a',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_010FA0': '7544336062064592bf10d24d932934201aa38cf2dba285e7fcedd71465847388',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_0117A0': '04e71c741c76dc743113130fcb0bf88a15a0a4a403de1c9370241ce0b3bf6e8f',
 'scenes/shared/tokinoma_scene/tokinoma_sceneTex_011FA0': '7cf812ef59c47707c5e7174e2f81631c6d1cf58118b3eb8baed12d9555f962a4'}
NATIVE_TEXTURE_HASHES['objects/object_mjin_flash/gLightMedallionPlatformTex'] = '9b01cfb49dec3bf4c1b224f183da6b150f9371a1f738143cc35158bca3197350'
ATLAS_PIXEL_HASHES = {
    'e4a6306988c23c5ca229d46c32b0cc913681d5d0e03670ad0f50c28381434b4f',
    'f59f0ad6d0058f50a594a6b0825967434ef90af686f8fa7647848a61d7b6863d',
}
ACCEPTED_GEOMETRY = {
    'scenes/shared/tokinoma_scene/tokinoma_room_0DL_008128':
        (12553, '81936e2b320b0da5ddd1be0ab6fa245f2f1f0df9a0dce7a875b77bea0f215fbe'),
    'custom/temple_sages/annex_opa':
        (1122, '20fd25f46fd5c76fb8779ade33ee7eb5e2c7b4190d22a999122e07e9730ba6ee'),
    'custom/temple_sages/annex_xlu':
        (706, '7f39c404ab0334d71e5e965d8a7817fc6b0922a58815d9af56161b92958611e3'),
}
COLLISION = 'scenes/shared/tokinoma_scene/tokinoma_sceneCollisionHeader_0032F8'
COLLISION_LAYOUT_SHA = 'b93b66583a79e6f632a8d5e97ce001f036e2f3f4df6c004f8c000b57561c044d'
ORIGINAL_VISUAL_TEXTURES = {'alt/custom/temple_sages/chamber/rm_kja_kabe_01__UDIM_': '14fb82eb32d12eb77d149174c1d93543f4c9bca2b293714e3b1b1a2fd0239b3e',
 'alt/custom/temple_sages/chamber/rm_kja_kabe_02_4000': '7e80969b85ee7c3967b050d1105e671442ffe8c2ec5d3dcebbf85724500664be',
 'alt/custom/temple_sages/chamber/rm_kja_kabe_03_8000': '4f05f4ddacaab569afc34ec5b65528d47b983c2145ebdb6f9a712f4f9f09c6b9',
 'alt/custom/temple_sages/chamber/rm_kja_mark_01_C000': '3675f4755ad531081cbfc0b142f9286b771930ad00b86f8470b24074fa95bfa4',
 'alt/custom/temple_sages/chamber/rm_kja_water_02_13C00': '3c494ee4c0337786d8824a656ec59978a8548244faacbc50184f3a24f07a4a19',
 'alt/custom/temple_sages/chamber/rm_kja_water_02_13C00___Copie': '9483180721504807f59d5f583360568909ce292ce03ff6dae8537ed234240c3a',
 'alt/custom/temple_sages/chamber/rm_kja_water_04_16400': '0a8e8ddca83308b9b0b943442118bf5f27103499858c3b7250ba5b9e03ade2e7',
 'alt/custom/temple_sages/chamber/rm_kja_yuka_01_16C00': '83ec24507977fc7a9d0a42a50084635d2204fd1148787a6e850573a792cdea63',
 'alt/custom/temple_sages/pillar_texture_0': 'cd23bc4bf7ffa1b7207b67d4d2255bc3f39e4539aed3426c08ce3ed81c19b505',
 'alt/custom/temple_sages/pillar_texture_1': 'c20eaf45110ed30ec6a59bf74bb051b48bb51f9d53d75167dde747c20edbd5db',
 'alt/custom/temple_sages/pillar_texture_2': '14fb82eb32d12eb77d149174c1d93543f4c9bca2b293714e3b1b1a2fd0239b3e',
 'alt/custom/temple_sages/pillar_texture_3': '7e80969b85ee7c3967b050d1105e671442ffe8c2ec5d3dcebbf85724500664be',
 'alt/custom/temple_sages/portals/fire/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/fire/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/forest/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/forest/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/shadow/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/shadow/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/spirit/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/spirit/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/water/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/portals/water/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'alt/custom/temple_sages/texture/arcane_stone': 'b7161e25e6c5545fd45669bd3599f8c3c89c03834b0a48a8d1bce730f5371f17',
 'alt/custom/temple_sages/texture/tunnel_stone': '748c92397629ba9f2b6038e0d220d4ee9e28d1286ecb38d1e0edd0d6cc8da8ff',
 'alt/custom/temple_sages/texture/veil': '450c7cc924869a6cdbf94d8d500a6f16082d2ca33f17435114d4ab8e0ee56135',
 'custom/temple_sages/chamber/rm_kja_mark_01_C000': '3675f4755ad531081cbfc0b142f9286b771930ad00b86f8470b24074fa95bfa4',
 'custom/temple_sages/chamber/rm_kja_water_02_13C00': '3c494ee4c0337786d8824a656ec59978a8548244faacbc50184f3a24f07a4a19',
 'custom/temple_sages/chamber/rm_kja_yuka_01_16C00': '83ec24507977fc7a9d0a42a50084635d2204fd1148787a6e850573a792cdea63',
 'custom/temple_sages/portals/fire/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/fire/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/forest/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/forest/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/shadow/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/shadow/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/spirit/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/spirit/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/water/texture_detail': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/portals/water/texture_flow': '9706784b4f3544903b6bc5443989715a3305f71e944af0c266aac12320ec5438',
 'custom/temple_sages/texture/arcane_stone': 'b7161e25e6c5545fd45669bd3599f8c3c89c03834b0a48a8d1bce730f5371f17'}
HOST_VISUAL_TEXTURES = {'alt/scenes/shared/tokinoma_scene/mjin_flash_mode_0': '8ec95827d7b597b018b239bd9754b63d4e6f1501fe8ed6fc13c4389db17df66c',
 'alt/scenes/shared/tokinoma_scene/toki_dai_01_5400': 'be8e9c62388f10af8fd10ba6fc23718212064f8c532c7adfb391696ef5f37eb3',
 'alt/scenes/shared/tokinoma_scene/toki_dai_02_7E00': '761e65d193eb5616a6bcd68bb479ad1dda7453f1bee6a13306e1960c112cbf05',
 'alt/scenes/shared/tokinoma_scene/toki_dai_03_D200': '1562d3f8a98ac5c1b171b86bbc613ccd3f63e64783b65f7e5e515755ff93ac90',
 'alt/scenes/shared/tokinoma_scene/toki_glass_01bx_12600': 'c61c78bf0301762e626d0eefbff18020c8e0aeb805a03858e8d263145627ec3a',
 'alt/scenes/shared/tokinoma_scene/toki_glass_01x_18A00': '257d226d50b64aaded604e61e75ca775824294fdf35a6c55891778617694405d',
 'alt/scenes/shared/tokinoma_scene/toki_glass_01xx_1B200': 'c380107fa55d85ed2c0636d5adf2548e801af17996a1351cfe0a58eec8e80f35',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_01_22B00': '9e3ed9e0775612c7fbc39ef66fc8b9f632057fe26126221343669d268b8109e4',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_02_25500': 'e51831e171755900a70ab2d12a2c0b56c90723659cbb38f60f0e6126b1771688',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_03_27F00': '358536fb8979d0b3a9364a25b5fa6c7073d55710063f0f97620ed74bd56b50eb',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_04_25500': 'a67f94bb0b3add9dd42a191c910f09487334cde44a29bd7b3d18d7d866206eff',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_05x_35100': 'ec7738d72a7e39c672e782e1105464c9cea2c954fb65aa75ce837964b4f6c2ac',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_06_32700': 'c1515de6802bc54a60e9a09ef908f6de63d5696fe45c27667927c4ee4c1cb1f7',
 'alt/scenes/shared/tokinoma_scene/toki_mark_01_39000': '927227439b46e74f51f4d6e2d1809275e66ebceffcc9ca90970d7b9b2209c3a2',
 'alt/scenes/shared/tokinoma_scene/toki_poal_01_39000': '962a3b62c59f62439fd7bf620df74bb8b32c14889680fe5ecf453ef7782e7a57',
 'alt/scenes/shared/tokinoma_scene/toki_saku_01_41880': '1f13ee8c8b6ac9004686987dbd03a102946705b9901aeca526a3ffe23cc0d292',
 'alt/scenes/shared/tokinoma_scene/toki_step_01_42280': '633bc878729b58ed73a1665b426d3acc21cbaad1ec82b61d14bd23a23462c3d5',
 'alt/scenes/shared/tokinoma_scene/toki_ten_01x_44C80': '75cc2bdfb63e085f76a560236fa2b5437e8717f9686a2fb458453360fea5e74e',
 'alt/scenes/shared/tokinoma_scene/toki_ten_02_40880': '6fae899dde45275d87667a390681d2a15d9487ac3a69558a4b953d063aaef4eb',
 'alt/scenes/shared/tokinoma_scene/toki_ten_04_46480': '41b79e09d6fd609c660e7cb40a9d0e4ce60319f49a9778240ad214e5dd8cd10a',
 'alt/scenes/shared/tokinoma_scene/toki_try_01_47480': '8081c22f6e94bc7740e9697a3bd7164290c84767b4b5947497a09c4e06892a45',
 'alt/scenes/shared/tokinoma_scene/toki_try_02_47F00': 'baf3e5764018a333c9131e1e21dfe23d18a5a3162357381f6cf5dd1cc9593c11',
 'alt/scenes/shared/tokinoma_scene/toki_try_03_48980': '09e34be168cda22f471dbb6bd750ec52498f45b5793f96d28720edddeed3b736',
 'alt/scenes/shared/tokinoma_scene/toki_yuka_01_44A80': '258b4ace3a42381d82c329fc9522204e1647fb381349b796d98cffbc5943b132',
 'alt/scenes/shared/tokinoma_scene/toki_yuka_02x_47480': 'd4b52b74e7dd78b37658963a030e89eb642285a655f56cc21edd82124f160e81',
 'alt/scenes/shared/tokinoma_scene/toki_yuka_03_50780': '944a4a5355055939de73214c8f4c8b13b35522b50af7d1255a035d24e870345c',
 'alt/scenes/shared/tokinoma_scene/toki_yuka_04_51201': '1534c37fa06918c59d24e51d4e8aeef3adb1b6c47b93b75d81c9934498f98b44'}
HOST_VISUAL_RESOURCES = {'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_00': '3d6795fc46482eab49ab740fe480897832a7bcafb8f2adef0f561d60c813d16b',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_01': '6c647f32f4f0385de1cbda18db108fbd1f63b1ec0ad2de49f385ba627494d00a',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_02': 'ffd0128c8b1bde28324ff58181f1c57e214c92d0530493e6b21ad573636f1fbf',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_03': '65de0ea031cc7e24d494ca94aa7902b93c1b792c700481c3102621dc156bfdd4',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_04': 'bf5f71416b4ee822c8d870a171471007c4c222a4aecf5c6a052aedb4bd26074b',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_05': '9a1e7f0a91cc3a1ab6cc3ee71854ab034ed3e2f14c6102fde3a4675f3fdace66',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_06': '1307f6a67e793c05b93487e4bef56872b6a59c3baf1adbfae67db2df4618f7fa',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_07': 'd9b8ca1adf3703a152959a87d751fc0ecf814639aac3a22dc7e811442188f7cc',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_08': '4a30644656b971d52bb182f3a047825a38be2ac8d117275bb8a62ff71f4581c0',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_09': '260f1f089e7279c8a504c01dc4310f96eb97c16bd5683c5fe8a1a37560cd4dca',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_10': 'ce7bec0ea096e58f5337c377bddc73b84ffa8445d02b9af0c118e4ec03ff80a7',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_11': '8b09f3605f519898d915d27be1fd175d41120c28a4b6be3fec072e5039957c9b',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_12': '1cec263a54ce149e4fd9f14c2498f48f16750ca8745cd85805223555c61a050b',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_13': '8fd8f2182b13c99c58f95611e84e8cf47b0f2de07a38981795cf1bad8a74ddbc',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_15': '75a4e471c461b40e23d9f7b78f7726edea14d3e51c25463de73b195672cf98f0',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_17': 'e21f043c5664b4a09d839bbb5ecf70515c152c2ea31883e3c887987055fe6953',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_18': 'b09555cddf8dac3cdb9ed57295132403147a52df8ab41e13e29f594e7473576a',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_19': 'bfb495b04c468cc2e2f34716b62c1503103d3eb4017017fe09c7400b8c9bd820',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_21': 'cfc4ca343cc743234325033561ea84dc6018db6759f5187c20e680f7ca572107',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_22': 'ce424ab083b5b7b9684e314bb4f0a42d8b338fa9b667944cbac51165b96837a8',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_23': '30da730d943cd81c0654acf85372340840e84debf6be0dc62e4bc6d0d1fa0b30',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_24': '246a01fac47ced6ac37ee9dde2190a5ec2c7e28f2087c2b8559a8a317e1cfb16',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_30': '1067d61fb70120a57c8cb153d53c0a53a5f34bdd438b5c5824de5c20897e35c1',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_31': '05edf00699dc3584227bb8726e231c4e5e669dad4bc3d67e80170b1f2a30240c',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_Time_33': '89d150017f825f90e9d9ce5c5e5e11e2348e1962b427cbeba94a1603a6a212de',
 'alt/scenes/shared/tokinoma_scene/mat_tokinoma_room_0DL_008128_Temple_of_time_25': 'e69ce49e0857fa360893c8363adc6080142177ab75913a4ee618147abc1c699d',
 'alt/scenes/shared/tokinoma_scene/mjin_flash_mode_0': '8ec95827d7b597b018b239bd9754b63d4e6f1501fe8ed6fc13c4389db17df66c',
 'alt/scenes/shared/tokinoma_scene/toki_dai_01_5400': 'be8e9c62388f10af8fd10ba6fc23718212064f8c532c7adfb391696ef5f37eb3',
 'alt/scenes/shared/tokinoma_scene/toki_dai_02_7E00': '761e65d193eb5616a6bcd68bb479ad1dda7453f1bee6a13306e1960c112cbf05',
 'alt/scenes/shared/tokinoma_scene/toki_dai_03_D200': '1562d3f8a98ac5c1b171b86bbc613ccd3f63e64783b65f7e5e515755ff93ac90',
 'alt/scenes/shared/tokinoma_scene/toki_glass_01bx_12600': 'c61c78bf0301762e626d0eefbff18020c8e0aeb805a03858e8d263145627ec3a',
 'alt/scenes/shared/tokinoma_scene/toki_glass_01x_18A00': '257d226d50b64aaded604e61e75ca775824294fdf35a6c55891778617694405d',
 'alt/scenes/shared/tokinoma_scene/toki_glass_01xx_1B200': 'c380107fa55d85ed2c0636d5adf2548e801af17996a1351cfe0a58eec8e80f35',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_01_22B00': '9e3ed9e0775612c7fbc39ef66fc8b9f632057fe26126221343669d268b8109e4',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_02_25500': 'e51831e171755900a70ab2d12a2c0b56c90723659cbb38f60f0e6126b1771688',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_03_27F00': '358536fb8979d0b3a9364a25b5fa6c7073d55710063f0f97620ed74bd56b50eb',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_04_25500': 'a67f94bb0b3add9dd42a191c910f09487334cde44a29bd7b3d18d7d866206eff',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_05x_35100': 'ec7738d72a7e39c672e782e1105464c9cea2c954fb65aa75ce837964b4f6c2ac',
 'alt/scenes/shared/tokinoma_scene/toki_kabe_06_32700': 'c1515de6802bc54a60e9a09ef908f6de63d5696fe45c27667927c4ee4c1cb1f7',
 'alt/scenes/shared/tokinoma_scene/toki_mark_01_39000': '927227439b46e74f51f4d6e2d1809275e66ebceffcc9ca90970d7b9b2209c3a2',
 'alt/scenes/shared/tokinoma_scene/toki_poal_01_39000': '962a3b62c59f62439fd7bf620df74bb8b32c14889680fe5ecf453ef7782e7a57',
 'alt/scenes/shared/tokinoma_scene/toki_saku_01_41880': '1f13ee8c8b6ac9004686987dbd03a102946705b9901aeca526a3ffe23cc0d292',
 'alt/scenes/shared/tokinoma_scene/toki_step_01_42280': '633bc878729b58ed73a1665b426d3acc21cbaad1ec82b61d14bd23a23462c3d5',
 'alt/scenes/shared/tokinoma_scene/toki_ten_01x_44C80': '75cc2bdfb63e085f76a560236fa2b5437e8717f9686a2fb458453360fea5e74e',
 'alt/scenes/shared/tokinoma_scene/toki_ten_02_40880': '6fae899dde45275d87667a390681d2a15d9487ac3a69558a4b953d063aaef4eb',
 'alt/scenes/shared/tokinoma_scene/toki_ten_04_46480': '41b79e09d6fd609c660e7cb40a9d0e4ce60319f49a9778240ad214e5dd8cd10a',
 'alt/scenes/shared/tokinoma_scene/toki_try_01_47480': '8081c22f6e94bc7740e9697a3bd7164290c84767b4b5947497a09c4e06892a45',
 'alt/scenes/shared/tokinoma_scene/toki_try_02_47F00': 'baf3e5764018a333c9131e1e21dfe23d18a5a3162357381f6cf5dd1cc9593c11',
 'alt/scenes/shared/tokinoma_scene/toki_try_03_48980': '09e34be168cda22f471dbb6bd750ec52498f45b5793f96d28720edddeed3b736',
 'alt/scenes/shared/tokinoma_scene/toki_yuka_01_44A80': '258b4ace3a42381d82c329fc9522204e1647fb381349b796d98cffbc5943b132',
 'alt/scenes/shared/tokinoma_scene/toki_yuka_02x_47480': 'd4b52b74e7dd78b37658963a030e89eb642285a655f56cc21edd82124f160e81',
 'alt/scenes/shared/tokinoma_scene/toki_yuka_03_50780': '944a4a5355055939de73214c8f4c8b13b35522b50af7d1255a035d24e870345c',
 'alt/scenes/shared/tokinoma_scene/toki_yuka_04_51201': '1534c37fa06918c59d24e51d4e8aeef3adb1b6c47b93b75d81c9934498f98b44'}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def effective(resources, alternate):
    selected = {name: data for name, data in resources.items() if not name.startswith('alt/')}
    if alternate:
        selected.update({name[4:]: data for name, data in resources.items() if name.startswith('alt/')})
    return selected


def collision_layout(data):
    if data[4:8] != b'LOCO':
        raise AssertionError('not a collision resource')
    offset = 76
    count = struct.unpack_from('<I', data, offset)[0]
    offset += 4
    vertices = [struct.unpack_from('<3h', data, offset + 6 * i) for i in range(count)]
    offset += count * 6
    count = struct.unpack_from('<I', data, offset)[0]
    offset += 4
    # Surface indices may change to select new stone sound/echo metadata.
    # All vertex flags/indices, normals and plane distances must stay exact.
    polygons = [struct.unpack_from('<4H4h', data, offset + 16 * i)[1:] for i in range(count)]
    offset += count * 16
    surfaces = struct.unpack_from('<I', data, offset)[0]
    offset += 4 + surfaces * 8
    return {'bounds': data[64:76].hex(), 'vertices': vertices,
            'polygons_without_audio_surface_index': polygons,
            'camera_water_tail': data[offset:].hex()}


class NativeChamberLayers(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path = ROOT / 'scripts/assets/prepare_temple_sages.py'
        spec = importlib.util.spec_from_file_location('native_layer_preparer', path)
        cls.preparer = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.preparer)
        cls.resources = cls.preparer.verified_resources()
        cls.metadata = json.loads(cls.preparer.DEFAULT_MANIFEST.read_text())

    def test_default_texture_payloads_are_native_bytes_or_lossless_native_atlases(self):
        self.assertEqual(self.metadata.get('texture_policy'), 'native-oot-base-optional-visual-addon')
        textures = {name: data for name, data in self.resources.items() if data[4:8] == b'XETO'}
        bindings = self.metadata.get('native_texture_bindings', {})
        self.assertTrue(textures, 'native base must have a verified texture inventory')
        self.assertEqual(set(bindings), set(textures))
        for name, data in textures.items():
            with self.subTest(resource=name):
                info = bindings[name]
                paths = info.get('native_paths', [])
                self.assertTrue(paths)
                self.assertTrue(all(path in NATIVE_TEXTURE_HASHES for path in paths))
                self.assertEqual(digest(data), info.get('resource_sha256'))
                if 'atlas_regions' not in info:
                    self.assertEqual(len(paths), 1)
                    self.assertEqual(digest(data), NATIVE_TEXTURE_HASHES[paths[0]],
                                     'a plain native binding must preserve its complete source bytes')
                else:
                    kind, width, height, flags, scale_x, scale_y, size = struct.unpack_from('<IIIIffI', data, 64)
                    self.assertEqual((kind, width, height, flags, scale_x, scale_y, size),
                                     (2, 128, 256, 1, 8.0, 4.0, 128 * 256 * 4))
                    self.assertEqual(len(info['atlas_regions']), 12)
                    self.assertIn(digest(data[92:]), ATLAS_PIXEL_HASHES,
                                  'atlas pixels/orientations must match independent native half-disc assembly')
                    self.assertEqual(len(data), 92 + size)

    def test_default_materials_have_no_authored_scrolling(self):
        for name, data in self.resources.items():
            if data.lstrip().startswith(b'<'):
                self.assertIsNone(ET.fromstring(data).find('.//ScrollTexture'), name)
            elif data[4:8] == b'TLDO':
                self.assertFalse(any(command >> 24 == 0x4b for _, command, _, _ in geometry.words(data)), name)

    def test_both_modes_retain_evaluated_geometry_and_all_uvs(self):
        for alternate in (False, True):
            resources = effective(self.resources, alternate)
            for path, (count, expected) in ACCEPTED_GEOMETRY.items():
                with self.subTest(alternate=alternate, root=path):
                    faces = [face for record in geometry.collect(resources, path) for face in record['faces']]
                    self.assertEqual(len(faces), count)
                    self.assertEqual(digest(json.dumps(faces, separators=(',', ':')).encode()), expected,
                                     'positions, UVs, vertex colors or primitive order changed')

    def test_collision_geometry_and_native_camera_water_are_unchanged(self):
        for alternate in (False, True):
            data = effective(self.resources, alternate)[COLLISION]
            actual = digest(json.dumps(collision_layout(data), separators=(',', ':')).encode())
            self.assertEqual(actual, COLLISION_LAYOUT_SHA)

    def test_optional_visual_archive_restores_art_and_scrolling_without_geometry_drift(self):
        configured = os.environ.get('TEMPLE_SAGES_VISUAL_ADDON')
        if not configured:
            self.skipTest('optional visual input is not configured; mandatory builtin checks remain active')
        path = Path(configured)
        self.assertTrue(path.is_file())
        with zipfile.ZipFile(path) as archive:
            visual = {name: archive.read(name) for name in archive.namelist() if not name.endswith('/')}
        textures = {name: digest(data) for name, data in visual.items() if data[4:8] == b'XETO'}
        self.assertEqual(textures, {**ORIGINAL_VISUAL_TEXTURES, **HOST_VISUAL_TEXTURES},
                         'the optional archive must retain existing art and include its host visual dependencies')
        for name, expected in HOST_VISUAL_RESOURCES.items():
            self.assertIn(name, visual, 'the visual addon still depends on an external host material pack')
            self.assertEqual(digest(visual[name]), expected)
        merged = dict(self.resources)
        merged.update(visual)
        active = effective(merged, True)
        self.assertGreater(sum(data.lstrip().startswith(b'<') and b'<ScrollTexture' in data
                               for data in active.values()), 10)
        for root, (count, expected) in ACCEPTED_GEOMETRY.items():
            faces = [face for record in geometry.collect(active, root) for face in record['faces']]
            self.assertEqual(len(faces), count)
            self.assertEqual(digest(json.dumps(faces, separators=(',', ':')).encode()), expected)
        self.assertEqual(digest(json.dumps(collision_layout(active[COLLISION]), separators=(',', ':')).encode()),
                         COLLISION_LAYOUT_SHA)


if __name__ == '__main__':
    unittest.main()
