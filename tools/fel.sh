#!/bin/bash
set -Eeuo pipefail

# --- Helpers ---------------------------------------------------------------
MB(){ echo $(( $1 * 1024 * 1024 )); }
ALIGN(){ local a=$1 v=$2; echo $(( ((v + a - 1) / a) * a )); }
hex(){ printf "0x%08x" "$1"; }

# --- SoC / RAM -------------------------------------------------------------
BOOT_ADDR=0x00028000
SDRAM_BASE=$((0x40000000))
SDRAM_SIZE_MB=128
SDRAM_TOP=$(( SDRAM_BASE + $(MB ${SDRAM_SIZE_MB}) ))

# --- Final layout (host-owned) --------------------------------------------
KERN_ADDR=$(( SDRAM_BASE + $(MB 32) ))            # 0x42000000
DTB_GUARD_MB=1
DTB_ADDR=$(( SDRAM_TOP - $(MB ${DTB_GUARD_MB}) )) # ~0x47F00000

# --- Mailboxes (awboot reads these; keep in sync) -------------------------
MAIL_INITRD_SIZE=$((0x43100000))   # uint32
MAIL_INITRD_START=$((0x43100004))  # uint32
MAIL_DTB_ADDR=$((0x43100008))      # uint32
MAIL_KERNEL_ADDR=$((0x4310000C))   # uint32 (optional but handy)

# --- Paths -----------------------------------------------------------------
if (( $# < 2 || $# > 3 )); then
  echo "Usage: $(basename "$0") <kernel> <dtb> [initrd]" >&2
  exit 1
fi

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
AWBOOT_BIN="${SCRIPT_DIR}/../build-fel/awboot-fel.bin"
KERNEL_FILE=${1}
DTB_FILE=${2}
INITRD_FILE=${3:-}

if [[ ! -f "${AWBOOT_BIN}" ]]; then
  echo "ERR: missing awboot binary at ${AWBOOT_BIN}" >&2
  exit 1
fi
if [[ ! -f "${KERNEL_FILE}" ]]; then
  echo "ERR: kernel file not found: ${KERNEL_FILE}" >&2
  exit 1
fi
if [[ ! -f "${DTB_FILE}" ]]; then
  echo "ERR: DTB file not found: ${DTB_FILE}" >&2
  exit 1
fi

# --- Sizes -----------------------------------------------------------------
KERNEL_SIZE_DEC=$(stat -c "%s" "${KERNEL_FILE}")
DTB_SIZE_DEC=$(stat -c "%s" "${DTB_FILE}")
INITRD_PRESENT=false
INITRD_SIZE_DEC=0
INITRD_SIZE_AL=0
if [[ -n "${INITRD_FILE}" ]]; then
  if [[ ! -f "${INITRD_FILE}" ]]; then
    echo "ERR: initrd file not found: ${INITRD_FILE}" >&2
    exit 1
  fi
  INITRD_PRESENT=true
  INITRD_SIZE_DEC=$(stat -c "%s" "${INITRD_FILE}")
  INITRD_SIZE_AL=$(ALIGN 64 ${INITRD_SIZE_DEC})     # must match awboot’s sanity alignment (64B)
fi

# --- Compute final addresses ----------------------------------------------
INITRD_END=${DTB_ADDR}                              # end is DTB addr (exclusive)
INITRD_START=0
if ${INITRD_PRESENT}; then
  INITRD_START=$(( INITRD_END - INITRD_SIZE_AL ))
fi

# --- Sanity checks ---------------------------------------------------------
if (( DTB_SIZE_DEC > $(MB ${DTB_GUARD_MB}) )); then
  echo "ERR: DTB (${DTB_SIZE_DEC} bytes) exceeds ${DTB_GUARD_MB} MiB guard under RAM top"
  exit 1
fi
if ${INITRD_PRESENT}; then
  if (( INITRD_START < SDRAM_BASE )); then
    echo "ERR: initrd start below SDRAM base ($(hex ${INITRD_START}))"
    exit 1
  fi
  if (( INITRD_END > SDRAM_TOP )); then
    echo "ERR: initrd end above SDRAM top ($(hex ${INITRD_END}))"
    exit 1
  fi
  # keep headroom between kernel and initrd
  if (( INITRD_START <= (KERN_ADDR + $(MB 16)) )); then
    echo "ERR: initrd too low, potential overlap with kernel/decompress area"
    exit 1
  fi
fi

# --- Map preview -----------------------------------------------------------
echo "Final (host-decided) memory map:"
printf "  zImage   @ %s (%s, %d bytes)\n"  "$(hex ${KERN_ADDR})" "$(basename "${KERNEL_FILE}")" ${KERNEL_SIZE_DEC}
printf "  DTB      @ %s (%s, %d bytes)\n"  "$(hex ${DTB_ADDR})"  "$(basename "${DTB_FILE}")"   ${DTB_SIZE_DEC}
if ${INITRD_PRESENT}; then
  printf "  initrd   @ %s .. %s (%d bytes, aligned %d)\n" \
         "$(hex ${INITRD_START})" "$(hex ${INITRD_END})" ${INITRD_SIZE_DEC} ${INITRD_SIZE_AL}
else
  echo "  initrd   disabled"
fi
printf "  mailboxes: initrd_size=%s initrd_start=%s dtb_addr=%s kernel_addr=%s\n" \
       "$(hex ${MAIL_INITRD_SIZE})" "$(hex ${MAIL_INITRD_START})" "$(hex ${MAIL_DTB_ADDR})" "$(hex ${MAIL_KERNEL_ADDR})"

# --- Push over FEL ---------------------------------------------------------
xfel ddr   t113-s3
xfel write ${BOOT_ADDR}                        "${AWBOOT_BIN}"
xfel write 0x$(printf %x ${KERN_ADDR})         "${KERNEL_FILE}"
xfel write 0x$(printf %x ${DTB_ADDR})          "${DTB_FILE}"
if ${INITRD_PRESENT}; then
  xfel write 0x$(printf %x ${INITRD_START})      "${INITRD_FILE}"
fi

# Mailboxes (tell awboot exactly where things are)
xfel write32 0x$(printf %x ${MAIL_INITRD_SIZE})  0x$(printf %x ${INITRD_SIZE_DEC})
xfel write32 0x$(printf %x ${MAIL_INITRD_START}) 0x$(printf %x ${INITRD_START})
xfel write32 0x$(printf %x ${MAIL_DTB_ADDR})     0x$(printf %x ${DTB_ADDR})
xfel write32 0x$(printf %x ${MAIL_KERNEL_ADDR})  0x$(printf %x ${KERN_ADDR})

# Optional verification (uncomment to debug)
# TMP="$(mktemp)"
# xfel read  0x$(printf %x ${DTB_ADDR})     ${DTB_SIZE_DEC}     "${TMP}"; cmp -n ${DTB_SIZE_DEC}     "${TMP}" "${DTB_FILE}"     || { echo "DTB mismatch"; exit 1; }
# xfel read  0x$(printf %x ${KERN_ADDR})    ${KERNEL_SIZE_DEC}  "${TMP}"; cmp -n ${KERNEL_SIZE_DEC}  "${TMP}" "${KERNEL_FILE}"  || { echo "Kernel mismatch"; exit 1; }
# xfel read  0x$(printf %x ${INITRD_START}) ${INITRD_SIZE_DEC}  "${TMP}"; cmp -n ${INITRD_SIZE_DEC}  "${TMP}" "${INITRD_FILE}"  || { echo "Initrd mismatch"; exit 1; }

xfel exec    ${BOOT_ADDR}
