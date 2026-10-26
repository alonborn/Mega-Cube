package com.megacube.controller

import org.junit.Assert.*
import org.junit.Test

class AnimationCatalogTest {
    @Test fun currentCatalogHasUniqueValidIdsAndNames() {
        val entries = AnimationCatalog.choices
        assertEquals(entries.size, entries.map { it.id }.toSet().size)
        entries.forEach { AnimationCatalog.packets(it) }
        assertEquals((40..51).toSet(), AnimationCatalog.additions(40).map { it.id }.toSet())
        assertEquals((38..51).toSet(), AnimationCatalog.additions(38).map { it.id }.toSet())
    }

    @Test fun removesRetiredAnimationsFromLearnedCatalogOnly() {
        val transfers = AnimationCatalog.syncEntries(40)
        val removals = transfers.filter { it.name.isEmpty() }
        assertTrue(removals.isEmpty())
        assertEquals(AnimationCatalog.additions(40), transfers.filter { it.name.isNotEmpty() })
        assertEquals(listOf(AnimationChoice(50, "YMCA Full +S"),
            AnimationChoice(45, "YMCA Perimeter"),
            AnimationChoice(46, "Wall Color Hold"),
            AnimationChoice(47, "Edge Color Sequence"),
            AnimationChoice(48, "Stage Build"),
            AnimationChoice(49, "Edge Color Sequence 2"),
            AnimationChoice(51, "YMCA Hip Sway")), AnimationCatalog.syncEntries(45))
        assertEquals((43..51).toSet(), AnimationCatalog.syncEntries(43).map { it.id }.toSet())
        assertEquals(listOf(AnimationChoice(50, "YMCA Full +S"),
            AnimationChoice(AnimationCatalog.WALL_DOUBLE_FLASH, "Wall Color Hold"),
            AnimationChoice(AnimationCatalog.EDGE_COLOR_SEQUENCE, "Edge Color Sequence"),
            AnimationChoice(48, "Stage Build"),
            AnimationChoice(49, "Edge Color Sequence 2"),
            AnimationChoice(51, "YMCA Hip Sway")),
            AnimationCatalog.syncEntries(46))
        assertEquals(listOf(AnimationChoice(50, "YMCA Full +S"), AnimationChoice(48, "Stage Build"),
            AnimationChoice(49, "Edge Color Sequence 2"),
            AnimationChoice(51, "YMCA Hip Sway")), AnimationCatalog.syncEntries(48))
        assertEquals(listOf(AnimationChoice(50, "YMCA Full +S"),
            AnimationChoice(AnimationCatalog.EDGE_COLOR_SEQUENCE2, "Edge Color Sequence 2"),
            AnimationChoice(51, "YMCA Hip Sway")),
            AnimationCatalog.syncEntries(49))
        assertEquals(listOf(AnimationChoice(50, "YMCA Full +S"),
            AnimationChoice(51, "YMCA Hip Sway")), AnimationCatalog.syncEntries(50))
        assertEquals(listOf(AnimationChoice(51, "YMCA Hip Sway")), AnimationCatalog.syncEntries(51))
        assertTrue(AnimationCatalog.syncEntries(52).isEmpty())
        assertArrayEquals(byteArrayOf(65, 67, 1, 40, 0, 0), AnimationCatalog.deletePacket(40))
        assertThrows(IllegalArgumentException::class.java) { AnimationCatalog.deletePacket(256) }
    }

    @Test fun newAnimationUsesDefaultMtuAndPreservesIdAndFullName() {
        val name = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456"
        val packets = AnimationCatalog.packets(AnimationChoice(255, name))
        assertEquals(listOf(20, 20, 10), packets.map { it.size })
        packets.forEachIndexed { i, packet ->
            assertArrayEquals(byteArrayOf(65, 67, 1, 255.toByte(), (i * 14).toByte(), 32), packet.take(6).toByteArray())
        }
        assertEquals(name, packets.flatMap { it.drop(6) }.toByteArray().toString(Charsets.US_ASCII))
    }

    @Test fun soundMarkerIsNotPartOfLcdName() {
        assertEquals("Future Show", AnimationCatalog.packets(AnimationChoice(40, "Future Show +S"))
            .flatMap { it.drop(6) }.toByteArray().toString(Charsets.US_ASCII))
    }

    @Test fun rejectsInvalidMetadata() {
        listOf(AnimationChoice(256, "Bad"), AnimationChoice(-1, "Bad"),
            AnimationChoice(40, ""), AnimationChoice(40, "A".repeat(33)),
            AnimationChoice(40, "Bad\nName"), AnimationChoice(40, "\u05d0")).forEach {
            assertThrows(IllegalArgumentException::class.java) { AnimationCatalog.packets(it) }
        }
    }
}
