package com.megacube.controller

data class AnimationChoice(val id: Int, val name: String)

object AnimationCatalog {
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
        AnimationChoice(40, "Walking Man"),
    )

    fun additions(defaultCount: Int): List<AnimationChoice> {
        require(defaultCount in 0..256)
        return choices.filter { it.id >= defaultCount }
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
