package com.megacube.controller

data class AnimationChoice(val id: Int, val name: String)

object AnimationCatalog {
    const val WALL_DOUBLE_FLASH = 46
    const val EDGE_COLOR_SEQUENCE = 47
    const val EDGE_COLOR_SEQUENCE2 = 49
    val choices = listOf(
        AnimationChoice(32, "Blackout"),
        AnimationChoice(5, "Fireworks +S"),
        AnimationChoice(19, "The Matrix +S"),
        AnimationChoice(25, "Electric Storm +S"),
        AnimationChoice(27, "Eye of Sauron +S"),
        AnimationChoice(30, "Opening +S"),
        AnimationChoice(31, "Opening 2 +S"),
        AnimationChoice(34, "Light Chase +S"),
        AnimationChoice(36, "Universal +S"),
        AnimationChoice(12, "Sinus +S"),
        AnimationChoice(50, "YMCA Full +S"),
        AnimationChoice(38, "Music Balls (LIVE)"),
        AnimationChoice(39, "Music Equalizer (LIVE)"),
        AnimationChoice(3, "Atoms"),
        AnimationChoice(6, "Helix 1"),
        AnimationChoice(14, "Starfield"),
        AnimationChoice(20, "Spotted Sphere"),
        AnimationChoice(21, "Supernova"),
        AnimationChoice(22, "Aurora"),
        AnimationChoice(23, "Black Hole"),
        AnimationChoice(24, "Metaballs"),
        AnimationChoice(26, "DNA Tunnel"),
        AnimationChoice(28, "White Test"),
        AnimationChoice(29, "Red Test"),
        AnimationChoice(33, "Helix 2"),
        AnimationChoice(35, "DeLorean Flight"),
        AnimationChoice(37, "Yoda"),
        AnimationChoice(40, "YMCA Letters"),
        AnimationChoice(41, "YMCA Side Steps"),
        AnimationChoice(42, "YMCA March"),
        AnimationChoice(43, "YMCA Disco"),
        AnimationChoice(44, "YMCA Clap"),
        AnimationChoice(45, "YMCA Perimeter"),
        AnimationChoice(WALL_DOUBLE_FLASH, "Wall Color Hold"),
        AnimationChoice(EDGE_COLOR_SEQUENCE, "Edge Color Sequence"),
        AnimationChoice(48, "Stage Build"),
        AnimationChoice(EDGE_COLOR_SEQUENCE2, "Edge Color Sequence 2"),
        AnimationChoice(51, "YMCA Hip Sway"),
    )

    private val retiredIds = (40..45).toSet()

    fun additions(defaultCount: Int): List<AnimationChoice> {
        require(defaultCount in 0..256)
        return choices.filter { it.id >= defaultCount }
    }

    fun syncEntries(defaultCount: Int): List<AnimationChoice> = additions(defaultCount) +
        retiredIds.filter { id -> id >= defaultCount && choices.none { it.id == id } }
            .map { AnimationChoice(it, "") }

    fun deletePacket(id: Int): ByteArray {
        require(id in 0..255)
        return byteArrayOf('A'.code.toByte(), 'C'.code.toByte(), 1, id.toByte(), 0, 0)
    }

    fun packets(choice: AnimationChoice): List<ByteArray> {
        require(choice.id in 0..255)
        val name = choice.name.removeSuffix(" +S").trim()
        require(name.length in 1..32 && name.all { it.code in 32..126 })
        val bytes = name.toByteArray(Charsets.US_ASCII)
        return bytes.asList().chunked(14).mapIndexed { index, part ->
            byteArrayOf('A'.code.toByte(), 'C'.code.toByte(), 1, choice.id.toByte(),
                (index * 14).toByte(), bytes.size.toByte()) + part.toByteArray()
        }
    }
}
