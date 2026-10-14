# Panduan Kontribusi (Contributing Guide)

Terima kasih atas ketertarikan Anda untuk berkontribusi pada proyek **ESP32 IoT Monitoring Dashboard**! 🚀

## Cara Berkontribusi

1. **Fork Repositori** ke akun GitHub Anda.
2. **Clone Fork Anda** ke komputer lokal:
   ```bash
   git clone https://github.com/dhidhin/esp32-dashboard.git
   cd esp32-dashboard
   ```
3. **Buat Branch Fitur Baru**:
   ```bash
   git checkout -b fitur/nama-fitur-anda
   ```
4. **Lakukan Perubahan dan Pengujian**:
   - Pasang dependensi: `npm install`
   - Jalankan dev server: `npm run dev`
   - Pastikan linter bersih: `npm run lint`
   - Pastikan build berhasil: `npm run build`
5. **Commit Perubahan Anda** dengan pesan commit yang jelas:
   ```bash
   git commit -m "feat: tambah filter rentang waktu pada grafik"
   ```
6. **Push ke Branch Anda**:
   ```bash
   git push origin fitur/nama-fitur-anda
   ```
7. **Buka Pull Request (PR)** pada branch `main` repositori ini.

## Standar Kode & Etika
- Gunakan pesan commit yang deskriptif (misal: `feat:`, `fix:`, `docs:`, `refactor:`).
- Jangan memasukkan kredensial pribadi atau file `.env` ke dalam commit.
- Hormati sesama kontributor dan ciptakan lingkungan yang kolaboratif.
