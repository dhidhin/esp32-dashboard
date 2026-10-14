import { useEffect, useState } from 'react'
import { supabase } from './lib/supabase'
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
  Legend
} from 'recharts'
import './App.css'

function App() {
  const [data, setData] = useState([])
  const [loading, setLoading] = useState(true)
  const [realtimeStatus, setRealtimeStatus] = useState('CONNECTING')

  async function loadData() {
    const { data: readings, error } = await supabase
      .from('sensor_readings')
      .select('*')
      .order('created_at', { ascending: false })
      .limit(30)

    if (error) {
      console.error(error)
      return
    }

    setData(readings.reverse())
    setLoading(false)
  }

  useEffect(() => {
    loadData()

    const channel = supabase
      .channel('sensor-readings-realtime')
      .on(
        'postgres_changes',
        {
          event: 'INSERT',
          schema: 'public',
          table: 'sensor_readings'
        },
        (payload) => {
          console.log('Data baru:', payload.new)

          setData((current) => {
            const updated = [...current, payload.new]
            return updated.slice(-30)
          })
        }
      )
      .subscribe((status) => {
        console.log('Realtime:', status)
        setRealtimeStatus(status)
      })

    return () => {
      supabase.removeChannel(channel)
    }
  }, [])

  const latest = data.length > 0 ? data[data.length - 1] : null

  const temperature = latest
    ? Number(latest.suhu).toFixed(1)
    : '--'

  const humidity = latest
    ? Number(latest.kelembapan).toFixed(1)
    : '--'

  const lastUpdate = latest
    ? new Date(latest.created_at)
    : null

  const secondsAgo = lastUpdate
    ? Math.floor((Date.now() - lastUpdate.getTime()) / 1000)
    : null

  const isOnline =
    secondsAgo !== null && secondsAgo <= 60

  function temperatureStatus(value) {
    if (value === null) return '--'

    if (value < 20) return 'Dingin'
    if (value < 30) return 'Normal'
    if (value < 35) return 'Hangat'

    return 'Panas'
  }

  function humidityStatus(value) {
    if (value === null) return '--'

    if (value < 40) return 'Kering'
    if (value <= 70) return 'Normal'

    return 'Lembap'
  }

  const chartData = data.map((item) => ({
    time: new Date(item.created_at).toLocaleTimeString('id-ID', {
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    }),
    suhu: Number(item.suhu),
    kelembapan: Number(item.kelembapan)
  }))

  return (
    <div className="app">

      <header className="header">
        <div>
          <h1>ESP32 IoT Monitoring</h1>
          <p>Temperature & Humidity Dashboard</p>
        </div>

        <div
          className={`connection ${
            realtimeStatus === 'SUBSCRIBED'
              ? 'connected'
              : 'disconnected'
          }`}
        >
          <span className="dot"></span>
          {realtimeStatus === 'SUBSCRIBED'
            ? 'REALTIME'
            : realtimeStatus}
        </div>
      </header>

      <main className="container">

        {/* STAT CARDS */}

        <section className="cards">

          <div className="card">
            <div className="card-icon">🌡️</div>

            <div>
              <div className="label">SUHU</div>

              <div className="value">
                {temperature}
                <span> °C</span>
              </div>

              <div className="status">
                {latest
                  ? temperatureStatus(Number(latest.suhu))
                  : '--'}
              </div>
            </div>
          </div>

          <div className="card">
            <div className="card-icon">💧</div>

            <div>
              <div className="label">KELEMBAPAN</div>

              <div className="value">
                {humidity}
                <span> %</span>
              </div>

              <div className="status">
                {latest
                  ? humidityStatus(Number(latest.kelembapan))
                  : '--'}
              </div>
            </div>
          </div>

          <div className="card">
            <div className="card-icon">📡</div>

            <div>
              <div className="label">ESP32 STATUS</div>

              <div
                className={`value ${
                  isOnline ? 'online' : 'offline'
                }`}
              >
                {isOnline ? 'ONLINE' : 'OFFLINE'}
              </div>

              <div className="status">
                {lastUpdate
                  ? `${secondsAgo} detik lalu`
                  : 'Belum ada data'}
              </div>
            </div>
          </div>

        </section>

        {/* CHART */}

        <section className="panel">

          <div className="panel-header">
            <div>
              <h2>Monitoring Sensor</h2>
              <p>30 pembacaan terakhir</p>
            </div>

            <div className="live-badge">
              LIVE
            </div>
          </div>

          <div className="chart">
            {loading ? (
              <div className="loading">
                Memuat data...
              </div>
            ) : (
              <ResponsiveContainer width="100%" height="100%">
                <LineChart data={chartData}>

                  <CartesianGrid strokeDasharray="3 3" />

                  <XAxis dataKey="time" />

                  <YAxis />

                  <Tooltip />

                  <Legend />

                  <Line
                    type="monotone"
                    dataKey="suhu"
                    name="Suhu (°C)"
                    stroke="#ef4444"
                    strokeWidth={3}
                    dot={false}
                  /> 

                  <Line
                    type="monotone"
                    dataKey="kelembapan"
                    name="Kelembapan (%)"
                    stroke="#2563eb"
                    strokeWidth={3}
                    dot={false}
                  />

                </LineChart>
              </ResponsiveContainer>
            )}
          </div>

        </section>

        {/* TABLE */}

        <section className="panel">

          <div className="panel-header">
            <div>
              <h2>Data Sensor</h2>
              <p>Pembacaan terbaru</p>
            </div>
          </div>

          <div className="table-wrapper">

            <table>

              <thead>
                <tr>
                  <th>Waktu</th>
                  <th>Suhu</th>
                  <th>Kelembapan</th>
                  <th>Status</th>
                </tr>
              </thead>

              <tbody>

                {[...data]
                  .reverse()
                  .slice(0, 10)
                  .map((item) => {

                    const temp = Number(item.suhu)

                    return (
                      <tr key={item.id}>

                        <td>
                          {new Date(
                            item.created_at
                          ).toLocaleString('id-ID')}
                        </td>

                        <td>
                          {temp.toFixed(1)} °C
                        </td>

                        <td>
                          {Number(
                            item.kelembapan
                          ).toFixed(1)} %
                        </td>

                        <td>
                          <span className="badge">
                            {temperatureStatus(temp)}
                          </span>
                        </td>

                      </tr>
                    )
                  })}

              </tbody>

            </table>

          </div>

        </section>

      </main>

      <footer>
        ESP32 + DHT22 + Supabase + React
      </footer>

    </div>
  )
}

export default App