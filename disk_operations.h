#pragma once

/* Internal drive identifier for the +3's built-in floppy drive. */
#define FDC_DRIVE 0

/* -------------------------------------------------------------------------- */
/* Timing                                                                     */
/* -------------------------------------------------------------------------- */

/* Busy-wait delay; safe to call from test logic. */
void delay_ms(unsigned int millis);

/* Optional callback pumped during coarse idle waits (delay_ms and command/result
 * phase FDC waits). Use NULL to disable. Never called during execution-phase
 * READ DATA byte transfers. */
void disk_operations_set_idle_pump(void (*pump)(void));

/* Read the ZX Spectrum 50 Hz frame counter (20 ms per tick). */
unsigned short frame_ticks(void);

/* -------------------------------------------------------------------------- */
/* Motor control                                                              */
/* -------------------------------------------------------------------------- */

/* Turns the motor on, waits for spin-up, and drains pending FDC interrupts. */
unsigned char plus3_motor_on(void);

/* Turns the motor off and waits for the drive to settle. */
void plus3_motor_off(void);

/* -------------------------------------------------------------------------- */
/* FDC commands                                                               */
/* -------------------------------------------------------------------------- */

typedef struct {
    /* Status result bytes returned by Read ID / Read Data (uPD765 ST0..ST2). */
    unsigned char st0;
    unsigned char st1;
    unsigned char st2;
} FdcStatus;

typedef struct {
    /* CHRN tuple from sector ID fields:
     * C=cylinder, H=head(side), R=record/sector number, N=size code. */
    unsigned char c;
    unsigned char h;
    unsigned char r;
    unsigned char n;
} FdcChrn;

typedef struct {
    /* Combined command result: status bytes + CHRN identity bytes. */
    FdcStatus status;
    FdcChrn chrn;
} FdcResult;

typedef struct {
    /* Sense Interrupt Status output after seek/recalibrate completes. */
    unsigned char st0;
    unsigned char pcn;
} FdcSeekResult;

/* Sense Drive Status: returns 1 on success, 0 on timeout. */
unsigned char cmd_sense_drive_status(unsigned char drive, unsigned char head,
                                     unsigned char *st3);

/* Poll until the drive reports READY (ST3 bit 5) or timeout. */
unsigned char wait_drive_ready(unsigned char drive, unsigned char head,
                               unsigned char *out_st3);

/* Recalibrate (seek to track 0). */
unsigned char cmd_recalibrate(unsigned char drive);

/* Seek to the given cylinder. */
unsigned char cmd_seek(unsigned char drive, unsigned char head,
                       unsigned char cyl);

/* Read ID: returns 1 when ST0/ST1/ST2 indicate clean success. */
unsigned char cmd_read_id(unsigned char drive, unsigned char head,
                          FdcResult *out_result);

/* Read a full sector into data[0..data_len). */
unsigned char cmd_read_data(unsigned char drive, unsigned char head,
                            unsigned char c, unsigned char h,
                            unsigned char r, unsigned char n,
                            FdcResult *out_result,
                            unsigned char *data, unsigned int data_len);

/* Wait for a seek or recalibrate to complete, then collect ST0/PCN. */
unsigned char wait_seek_complete(unsigned char drive, FdcSeekResult *out_result);

/* Byte count for a sector with the given N field (0-3 → 128-1024 bytes). */
unsigned int sector_size_from_n(unsigned char n);

/* Human-readable reason string for a Read ID / Read Data failure. */
const char *read_id_failure_reason(unsigned char st1, unsigned char st2);

/*
 * fdc_measure_revolutions_ticks()
 *
 * Times `num_revs` complete disk revolutions by issuing READ ID commands and
 * watching for a reference sector (first_r) to reappear after at least one
 * other sector has been seen.
 *
 * On a standard ZX Spectrum +3/+3DOS formatted disk each track has 9 sectors
 * (typically R=1..9). READ ID returns different sector numbers on successive
 * calls as the disk rotates, so seeing sectors other than first_r is expected
 * and required for revolution detection.
 *
 * No artificial delay is inserted between READ ID calls. The FDC blocks
 * naturally until the next sector ID mark passes the head (~22 ms at 300 RPM).
 * Inserting delays inflates the measured period and causes RPM under-reporting.
 *
 * seen_other is reset after each counted revolution to prevent double-counting
 * if first_r appears in consecutive READ ID results.
 *
 * Returns the total elapsed 20 ms frame ticks for num_revs revolutions.
 * Returns 0 on READ ID failure or if timeout_ticks elapses before completion.
 */
unsigned char fdc_measure_revolutions_ticks(unsigned char drive,
                                            unsigned char head,
                                            unsigned char num_revs,
                                            unsigned char timeout_ticks);

