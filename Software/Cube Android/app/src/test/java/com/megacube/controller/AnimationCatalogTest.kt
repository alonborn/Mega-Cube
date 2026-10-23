package com.megacube.controller

import org.junit.Assert.*
import org.junit.Test

class AnimationCatalogTest {
    @Test fun currentCatalogHasUniqueValidIdsAndNames() {
        val entries = AnimationCatalog.choices
        assertEquals(entries.size, entries.map { it.id }.toSet().size)
        entries.forEach { AnimationCatalog.packets(it) }
        assertEquals(listOf(AnimationChoice(40, "Walking Man"), AnimationChoice(41, "Jumping Man"), AnimationChoice(42, "Running Man")), AnimationCatalog.additions(40))
        assertEquals(listOf(AnimationChoice(41, "Jumping Man"), AnimationChoice(42, "Running Man")), AnimationCatalog.additions(41))
        assertEquals(listOf(AnimationChoice(42, "Running Man")), AnimationCatalog.additions(42))
        assertTrue(AnimationCatalog.additions(43).isEmpty())
        assertEquals(setOf(38, 39, 40, 41, 42), AnimationCatalog.additions(38).map { it.id }.toSet())
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
