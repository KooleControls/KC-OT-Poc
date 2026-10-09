import { useMemo, useState, type ReactNode } from "react"
import { ActivityIcon, PauseIcon, PlayIcon, SendIcon, TrashIcon } from "lucide-react"
import { toast } from "sonner"

import { Badge } from "@/components/ui/badge"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"
import { useOpenThermFrames, type FrameRow } from "@/hooks/use-opentherm"
import { errorMessage } from "@/hooks/use-led"
import { backend, type ModuleLinkState, type OpenThermSide } from "@/lib/backend"
import { DATA_IDS, MSG_TYPES, decodeFrame, encodeFrame } from "@/lib/opentherm"

/**
 * The OpenTherm traffic through the PCB1246 module, as it happens.
 *
 * Every frame either line receives, and every frame the gateway puts on one, in one
 * table, newest first, decoded for reading. The decoding is this page's only: the
 * modem and the gateway pass frames through as numbers (lib/opentherm.ts).
 *
 * Below it, a frame composer for the bench: build a frame from its parts and put it
 * on a line with `module send`.
 */
export default function OpenThermPage() {
  const { rows, missed, link, error, clear } = useOpenThermFrames()
  const [paused, setPaused] = useState<FrameRow[] | null>(null)
  const [filter, setFilter] = useState<"all" | OpenThermSide>("all")

  // Paused means the VIEW stops; the hook keeps collecting underneath.
  const visible = useMemo(() => {
    const source = paused ?? rows
    return filter === "all" ? source : source.filter((r) => r.side === filter)
  }, [paused, rows, filter])

  return (
    <div className="flex h-full flex-col gap-4">
      <div className="flex flex-wrap items-center justify-between gap-3">
        <div className="flex items-center gap-2">
          <ActivityIcon className="text-muted-foreground size-5" />
          <h1 className="text-2xl font-bold">OpenTherm</h1>
          <LinkBadge link={link} />
          <span className="text-muted-foreground text-sm">
            {visible.length} frames{missed > 0 && `, ${missed} missed`}
          </span>
        </div>

        <div className="flex flex-wrap items-center gap-2">
          {(["all", "thermostat", "boiler"] as const).map((f) => (
            <Button
              key={f}
              size="sm"
              variant={filter === f ? "secondary" : "ghost"}
              onClick={() => setFilter(f)}
            >
              {f === "all" ? "Both lines" : capitalise(f)}
            </Button>
          ))}
          <Button
            size="sm"
            variant="outline"
            onClick={() => setPaused((p) => (p ? null : rows))}
          >
            {paused ? <PlayIcon /> : <PauseIcon />}
            {paused ? "Resume" : "Pause"}
          </Button>
          <Button size="sm" variant="outline" onClick={() => { clear(); setPaused(null) }}>
            <TrashIcon />
            Clear
          </Button>
        </div>
      </div>

      {error && <p className="text-destructive text-sm">Polling failed: {error}</p>}

      <Card className="min-h-0 flex-1 overflow-hidden py-0">
        <div className="h-full overflow-auto">
          <table className="w-full text-left font-mono text-xs">
            <thead className="bg-card text-muted-foreground sticky top-0 z-10">
              <tr className="border-b">
                <Th>Time</Th>
                <Th>Line</Th>
                <Th>Frame</Th>
                <Th>Type</Th>
                <Th>Data id</Th>
                <Th>Value</Th>
                <Th className="text-right">Δ ms</Th>
              </tr>
            </thead>
            <tbody>
              {visible.map((r) => (
                <FrameRowView key={r.seq + ":" + r.ms} row={r} />
              ))}
              {visible.length === 0 && (
                <tr>
                  <td colSpan={7} className="text-muted-foreground p-6 text-center font-sans text-sm">
                    {link?.phase === "ready"
                      ? "No frames yet. A thermostat on the module sends about once a second."
                      : "No frames - the link to the module is not up."}
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </Card>

      <Composer linkReady={link?.phase === "ready"} />
    </div>
  )
}

function FrameRowView({ row }: { row: FrameRow }) {
  const d = row.decoded
  // Who said it: off the line it is the device on that line; on it, the gateway.
  const route =
    row.dir === "rx" ? `${capitalise(row.side)} → GW` : `GW → ${capitalise(row.side)}`
  const typeTone =
    d.msgType === 6 || d.msgType === 7 || d.msgType === 2
      ? "text-amber-600 dark:text-amber-400"
      : d.fromMaster
        ? ""
        : "text-emerald-700 dark:text-emerald-400"

  return (
    <tr className={`border-b last:border-0 ${row.dir === "tx" ? "bg-muted/40" : ""}`}>
      <Td className="text-muted-foreground">{(row.ms / 1000).toFixed(3)}</Td>
      <Td className="whitespace-nowrap">{route}</Td>
      <Td>
        {d.hex}
        {!d.parityOk && (
          <Badge variant="destructive" className="ml-2">
            parity
          </Badge>
        )}
      </Td>
      <Td className={`whitespace-nowrap ${typeTone}`}>{d.msgTypeName}</Td>
      <Td className="whitespace-nowrap">
        <span className="text-muted-foreground">{d.dataId}</span> {d.dataIdName}
      </Td>
      <Td>{d.valueText}</Td>
      <Td className="text-muted-foreground text-right">{row.deltaMs ?? ""}</Td>
    </tr>
  )
}

function Composer({ linkReady }: { linkReady: boolean }) {
  const [side, setSide] = useState<OpenThermSide>("boiler")
  const [msgType, setMsgType] = useState(0)
  const [dataId, setDataId] = useState("0")
  const [value, setValue] = useState("0x0300")
  const [busy, setBusy] = useState(false)

  const id = parseNumber(dataId)
  const val = parseNumber(value)
  const valid = id !== null && id >= 0 && id <= 255 && val !== null && val >= 0 && val <= 0xffff
  const frame = valid ? encodeFrame(msgType, id, val) : null

  const send = () => {
    if (frame === null) return
    setBusy(true)
    backend
      .sendModuleFrame(side, frame)
      .then((r) => {
        if (r.ok) toast.success(`Sent ${decodeFrame(frame).hex} on the ${side} line`)
        else toast.error("Not sent", { description: r.error })
      })
      .catch((e) => toast.error("Not sent", { description: errorMessage(e) }))
      .finally(() => setBusy(false))
  }

  const selectClass =
    "border-input dark:bg-input/30 h-8 rounded-lg border bg-transparent px-2 text-sm"

  return (
    <Card className="shrink-0">
      <CardHeader>
        <CardTitle>Send a frame</CardTitle>
      </CardHeader>
      <CardContent className="flex flex-wrap items-end gap-3">
        <Field label="Line" htmlFor="ot-side">
          <select
            id="ot-side"
            className={selectClass}
            value={side}
            onChange={(e) => setSide(e.target.value as OpenThermSide)}
          >
            <option value="thermostat">Thermostat (as boiler)</option>
            <option value="boiler">Boiler (as thermostat)</option>
          </select>
        </Field>
        <Field label="Type" htmlFor="ot-type">
          <select
            id="ot-type"
            className={selectClass}
            value={msgType}
            onChange={(e) => setMsgType(Number(e.target.value))}
          >
            {MSG_TYPES.map((t, i) => (
              <option key={t} value={i}>
                {t}
              </option>
            ))}
          </select>
        </Field>
        <Field label="Data id" htmlFor="ot-id">
          <Input
            id="ot-id"
            className="w-20"
            value={dataId}
            onChange={(e) => setDataId(e.target.value)}
            aria-invalid={id === null || id < 0 || id > 255}
          />
        </Field>
        <Field label="Value (16 bit)" htmlFor="ot-value">
          <Input
            id="ot-value"
            className="w-28 font-mono"
            value={value}
            onChange={(e) => setValue(e.target.value)}
            aria-invalid={val === null || val < 0 || val > 0xffff}
          />
        </Field>
        <div className="text-muted-foreground min-w-48 pb-1.5 text-xs">
          {frame !== null ? (
            <>
              <span className="text-foreground font-mono">{decodeFrame(frame).hex}</span>{" "}
              {DATA_IDS[id!]?.name ?? `data id ${id}`}
            </>
          ) : (
            "Data id 0-255, value 0-0xFFFF"
          )}
        </div>
        <Button onClick={send} disabled={!valid || busy || !linkReady}>
          <SendIcon />
          Send
        </Button>
      </CardContent>
    </Card>
  )
}

function LinkBadge({ link }: { link: ModuleLinkState | null }) {
  if (!link) return <Badge variant="outline">link …</Badge>
  const text: Record<ModuleLinkState["phase"], string> = {
    ready: "module linked",
    handshake: "waiting for module",
    paused: "module in bootloader",
    failed: "link failed",
  }
  return (
    <Badge variant={link.phase === "ready" ? "secondary" : "destructive"}>
      {text[link.phase] ?? link.phase}
    </Badge>
  )
}

function Field({ label, htmlFor, children }: { label: string; htmlFor: string; children: ReactNode }) {
  return (
    <div className="flex flex-col gap-1">
      <Label htmlFor={htmlFor} className="text-muted-foreground text-xs">
        {label}
      </Label>
      {children}
    </div>
  )
}

function Th({ children, className = "" }: { children: ReactNode; className?: string }) {
  return <th className={`px-3 py-2 font-medium ${className}`}>{children}</th>
}

function Td({ children, className = "" }: { children: ReactNode; className?: string }) {
  return <td className={`px-3 py-1.5 align-top ${className}`}>{children}</td>
}

function capitalise(s: string): string {
  return s.charAt(0).toUpperCase() + s.slice(1)
}

/** Decimal or 0x-hex, whole numbers only; null when it is neither. */
function parseNumber(text: string): number | null {
  const t = text.trim()
  if (/^0x[0-9a-f]+$/i.test(t)) return parseInt(t.slice(2), 16)
  if (/^\d+$/.test(t)) return parseInt(t, 10)
  return null
}
