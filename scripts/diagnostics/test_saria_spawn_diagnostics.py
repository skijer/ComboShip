import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class ProgressionActorSpawnDiagnosticsTest(unittest.TestCase):
    def test_scene_entry_probe_records_saria_and_child_malon_before_spawn_hook(self):
        source = (ROOT / "soh/src/code/z_actor.c").read_text()
        function = source[source.index("Actor* Actor_SpawnEntry(") : source.index("Actor* Actor_Delete(")]

        self.assertIn("[ProgressionActorProbe] actor=%s stage=scene-entry", function)
        self.assertIn("actorEntry->id == ACTOR_EN_SA", function)
        self.assertIn("actorEntry->id == ACTOR_EN_MA1", function)
        self.assertIn("actorEntry->params", function)
        self.assertIn("actorEntry->pos.x", function)
        self.assertIn("GameInteractor_Should(VB_SPAWN_ACTOR_ENTRY", function)
        self.assertIn("[ProgressionActorProbe] actor=%s stage=spawn-hook", function)

    def test_init_probe_records_every_eligibility_input_and_kill(self):
        source = (ROOT / "soh/src/overlays/actors/ovl_En_Sa/z_en_sa.c").read_text()
        function = source[source.rindex("void EnSa_Init(Actor* thisx") : source.rindex("void EnSa_Destroy(")]

        for required in (
            "[SariaSpawnProbe] stage=init",
            "play->sceneNum",
            "gSaveContext.cutsceneIndex",
            "LINK_IS_ADULT",
            "IS_RANDO",
            "EVENTCHKINF_OBTAINED_ZELDAS_LETTER",
            "QUEST_SONG_SARIA",
            "initMode",
            "[SariaSpawnProbe] stage=kill",
        ):
            self.assertIn(required, function)

        self.assertIn("Actor_Kill(&this->actor)", function)

    def test_child_malon_probe_records_spawn_inputs_and_kill_reasons(self):
        source = (ROOT / "soh/src/overlays/actors/ovl_En_Ma1/z_en_ma1.c").read_text()
        init = source[source.rindex("void EnMa1_Init(Actor* thisx") : source.rindex("void EnMa1_Destroy(")]

        for required in (
            "[MalonSpawnProbe] stage=init",
            "play->sceneNum",
            "gSaveContext.sceneLayer",
            "LINK_IS_CHILD",
            "IS_DAY",
            "EVENTCHKINF_TALON_RETURNED_FROM_CASTLE",
            "INFTABLE_ENTERED_HYRULE_CASTLE",
            "QUEST_SONG_EPONA",
            "shouldSpawn",
            "[MalonSpawnProbe] stage=kill reason=eligibility-zero",
        ):
            self.assertIn(required, init)

        self.assertIn("[MalonSpawnProbe] stage=kill reason=send-home", source)


if __name__ == "__main__":
    unittest.main()
