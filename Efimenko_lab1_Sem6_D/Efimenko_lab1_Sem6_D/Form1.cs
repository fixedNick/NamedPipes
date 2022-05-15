using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Efimenko_lab1_Sem6_D
{
    public partial class Form1 : Form
    {
        #region Dll Methods To Make Requests And Get Answers To/From Server.
        [DllImport(@"..\..\..\..\x64\Debug\MMF.dll")]
        [return: MarshalAs(UnmanagedType.BStr)]
        public static extern string SendTextThroughNamedPipe(string fileText, [MarshalAs(UnmanagedType.I4)] int tidx);

        [DllImport(@"..\..\..\..\x64\Debug\MMF.dll")]
        [return: MarshalAs(UnmanagedType.I4)] // Возвращает количество запущенных потоков, если
        // Параметр threadsCount - сколько потоков запустить
        // ВАЖНО! При первом запуске - start инициализирует лишь главный поток
        // А при дальнейших будет запускать threadCount потоков
        public static extern int StartServerThread([MarshalAs(UnmanagedType.I4)] int threadsCount);

        [DllImport(@"..\..\..\..\x64\Debug\MMF.dll")]
        [return: MarshalAs(UnmanagedType.I4)] // Возвращает количество запущенных потоков, если
        // Если есть активные потоки, то завершает последний
        // Если остался только главный поток - то завершает его
        public static extern int StopServerThread([MarshalAs(UnmanagedType.Bool)] bool stopServer);

        [DllImport(@"..\..\..\..\x64\Debug\MMF.dll")]
        [return: MarshalAs(UnmanagedType.I4)]
        // Метод для подключения к пайпу
        // При успешном подключении к пайпу возвращает текущее число активных потоков
        // При неудаче возвращает -222
        public static extern int ConnectToNamedPipe();


        [DllImport(@"..\..\..\..\x64\Debug\MMF.dll")]
        // Оповещаем сервер о нашем отключении(о том, что клиент отключается)
        public static extern void ClientDisconnect();

        #endregion

        public Form1()
        {
            InitializeComponent();
        }

        private void StartButton_Click(object sender, EventArgs e)
        {
            int threadsToStart;
            if (int.TryParse(textBox1.Text, out threadsToStart) == false)
            {
                MessageBox.Show("Введите корректное число потоков для запуска!");
                return;
            }

            // Метод Start вернет 0, если сервер еще не был запущен
            // и не будет запускать N потоков
            // Поэтому проверим, что сервер действительно запущен
            var activeThreadsCount = StartServerThread(threadsToStart);
            UpdateThreadsUI(activeThreadsCount);
        }

        private void UpdateThreadsUI(int activeThreadsCount)
        {
            listBox1.Items.Clear();
                label2.Text = "0";
            /*if (activeThreadsCount == -1)
                label2.Text = "0";
            else */
            if (activeThreadsCount == 0)
            {
                listBox1.Items.Add($"Threads count: {activeThreadsCount}");
            }
            else if (activeThreadsCount > 0)
            {
                listBox1.Items.Add($"Threads count: {activeThreadsCount}");
                listBox1.Items.Add($"All threads");
                for (int i = 0; i < activeThreadsCount; i++)
                    listBox1.Items.Add($"Thread # {i + 1}");
                label2.Text = activeThreadsCount.ToString();
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            // Если потоков не осталось и закрывается главный поток - вернет -1
            // Что мы успешно обработаем в UpdateThreadsUI
            var activeThreadsCount = StopServerThread(false);
            UpdateThreadsUI(activeThreadsCount);

        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            // Вырубаем сервер
            ClientDisconnect();
        }

        private void TryConnectToNamedPipe()
        {
            // client disconnect ->
            // -> нужно для того, чтобы если клиент подключен по пайпу и все равно тыкает реконект - не было ошибк
            // при его будущем отключении
            ClientDisconnect();

            int threadsCount = ConnectToNamedPipe();
            if (threadsCount != -222)
            {
                UpdateThreadsUI(threadsCount);
                label5.Text = "status: CONNECTED";
            }
            else label5.Text = "status: NO CONNECTION";
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            TryConnectToNamedPipe();
        }

        // Send button
        private void button1_Click(object sender, EventArgs e)
        {
            int threadIdx = -2;
            // Если значение threadIdx = -1 - значит выбран пункт: All Threads. 
            // Номера потоков начинаются с 1.

            // ListBox.Items[0] - Threads Count
            // ListBox.Items[1] - All Threads
            // ListBox.Items[2..INFINITE] - [1..2..3..10..INFINITE] Thread
            // Чтобы получить доступ к списку Items, а точнее к выбранному пользователем номеру в списке
            // используем поле SelectedIndex у объекта нашего списка listBox1

            if (listBox1.SelectedIndex == 1)
                threadIdx = -1;
            else if (listBox1.SelectedIndex >= 2)
                threadIdx = listBox1.SelectedIndex;
            else
            {
                MessageBox.Show("Select thread in list to send text to");
                return;
            }

            threadIdx = threadIdx == -1 ? threadIdx : threadIdx - 2;

            var response = SendTextThroughNamedPipe(textBox2.Text, threadIdx);
            MessageBox.Show(response, "Ответ сервера");
            if (response == textBox2.Text)
                MessageBox.Show("Запись в файл(-ы) завершена!");
            else MessageBox.Show("При записи в файл(-ы) возникла ошибка");
        }

        private void button3_Click(object sender, EventArgs e)
        {
            TryConnectToNamedPipe();
        }
    }
}