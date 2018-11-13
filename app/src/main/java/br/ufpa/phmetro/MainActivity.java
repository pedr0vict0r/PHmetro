package br.ufpa.phmetro;


import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.util.TimerTask;


import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Timer;
import java.util.logging.Logger;

import br.ufpa.phmetro.ConnectionThread;
import br.ufpa.phmetro.FileStorage;

public class MainActivity extends Activity{

    /* Definição dos objetos que serão usados na Activity Principal
        statusMessage mostrará mensagens de status sobre a conexão
        counterMessage mostrará o valor do contador como recebido do Arduino
        connect é a thread de gerenciamento da conexão Bluetooth
     */
    public static int ENABLE_BLUETOOTH = 1;
    public static int SELECT_PAIRED_DEVICE = 2;
    public static int SELECT_DISCOVERED_DEVICE = 3;

    static TextView statusMessage;
    static TextView viewPH;
    static TextView acoes;

    public String data_completa;
    public Date data_atual;

    public Button registroalimentacao;
    public Button registrodeitar;
    public Button registrosintomas;

    public boolean alimento = false;
    public boolean deitar = false;

    ConnectionThread connect;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        /*
        String dir = Environment.getExternalStorageDirectory() + "/phmetro/recorder";
        File f = new File(dir);
        if(!f.isDirectory()) {
            File newdir = new File(dir);
            newdir.mkdirs();
        } */

        /* Link entre os elementos da interface gráfica e suas
            representações em Java.
         */
        statusMessage = (TextView) findViewById(R.id.statusMessage);

        viewPH = (TextView) findViewById(R.id.viewPH);

        acoes = (TextView) findViewById(R.id.acoes);

        registroalimentacao = (Button) findViewById(R.id.button_registroalimentacao);

        registrodeitar = (Button) findViewById(R.id.button_registrodeitar);

        registrosintomas = (Button) findViewById(R.id.button_registrosintomas);

        registroalimentacao.setOnClickListener(new Button.OnClickListener()
        {
            public void onClick(View v)
            {
                if (!alimento) {
                    acoes.setText("Alimentação iniciada");
                    alimento = true;
                    salvar(acoes.getText().toString());

                } else {
                    acoes.setText("Alimentação finalizada");
                    alimento = false;
                    salvar(acoes.getText().toString());
                }
            }
        }

        );

        registrodeitar.setOnClickListener(new Button.OnClickListener()
         {
             public void onClick(View v)
             {
                 if (!deitar) {
                     acoes.setText("Paciente deitado");
                     deitar = true;
                     salvar(acoes.getText().toString());
                 } else {
                     acoes.setText("Paciente em pé");
                     deitar = false;
                     salvar(acoes.getText().toString());
                 }
             }
         }

        );

        registrosintomas.setOnClickListener(new Button.OnClickListener()
        {
            public void onClick(View v)
            {
                acoes.setText("Registro de sintoma");
            }
        }
        );

        /* Teste rápido. O hardware Bluetooth do dispositivo Android
            está funcionando ou está bugado?
         */
        BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
        if (btAdapter == null) {
            statusMessage.setText("Que pena! Hardware Bluetooth não está funcionando :(");
        } else {
            statusMessage.setText("Ótimo! Hardware Bluetooth está funcionando :D");
        }

        if(!btAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, ENABLE_BLUETOOTH);
            statusMessage.setText("Solicitando ativação do Bluetooth...");
        } else {
            statusMessage.setText("Bluetooth já ativado :)");
        }

        /* Um descanso rápido, para evitar bugs.
         */
        try {
            Thread.sleep(1000);
        } catch (Exception E) {
            E.printStackTrace();
        }
    }

    public void tempo(){
        try{
            SimpleDateFormat dateFormat = new SimpleDateFormat("dd-MM-yyyy HH:mm:ss");
            // OU
            //SimpleDateFormat dateFormat_hora = new SimpleDateFormat("HH:mm:ss");

            Date data = new Date();

            Calendar cal = Calendar.getInstance();
            cal.setTime(data);
            data_atual = cal.getTime();

            data_completa = dateFormat.format(data_atual);

            //hora_atual = dateFormat_hora.format(data_atual);

            Log.i("data_completa", data_completa);
            //Log.i("data_atual", data_atual.toString());
            //Log.i("hora_atual", hora_atual); // Esse é o que você quer
        }catch (Exception e){

        }

    }

    public void salvar(String texto){
        tempo();
        try{
            if (FileStorage.saveToFile( texto + ";" + data_completa)){
                Toast.makeText(MainActivity.this,"Salvo com sucesso!",Toast.LENGTH_SHORT).show();
            }else{
                Toast.makeText(MainActivity.this,"Erro ao salvar!",Toast.LENGTH_SHORT).show();
            }
        }catch (Exception e) {
            Toast.makeText(MainActivity.this, "Erro ao salvar!", Toast.LENGTH_LONG).show();
        }
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        //getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        //if (id == R.id.action_settings) {
        //    return true;
        //}

        return super.onOptionsItemSelected(item);
    }


    public static Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {

            /* Esse método é invocado na Activity principal
                sempre que a thread de conexão Bluetooth recebe
                uma mensagem.
             */
            Bundle bundle = msg.getData();
            byte[] data = bundle.getByteArray("data");
            String dataString= new String(data);

            /* Aqui ocorre a decisão de ação, baseada na string
                recebida. Caso a string corresponda à uma das
                mensagens de status de conexão (iniciadas com --),
                atualizamos o status da conexão conforme o código.
             */
            if(dataString.equals("---N"))
                statusMessage.setText("Ocorreu um erro durante a conexão D:");
            else if(dataString.equals("---S"))
                statusMessage.setText("Conectado :D");
            else {
                if(dataString.contains("PH")){
                    viewPH.setText(dataString);
                }else{

                }
                /* Se a mensagem não for um código de status,
                    então ela deve ser tratada pelo aplicativo
                    como uma mensagem vinda diretamente do outro
                    lado da conexão. Nesse caso, simplesmente
                    atualizamos o valor contido no TextView do
                    contador.
                 */
            }

        }
    };


    /* Esse método é invocado sempre que o usuário clicar na TextView
        que contém o contador. O app Android transmite a string "restart",
        seguido de uma quebra de linha, que é o indicador de fim de mensagem.
     */
    public void restartCounter(View view) {
        salvar(viewPH.getText().toString());
    }

    public void searchPairedDevices(View view) {

        Intent searchPairedDevicesIntent = new Intent(this, PairedDevices.class);
        startActivityForResult(searchPairedDevicesIntent, SELECT_PAIRED_DEVICE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

        if(requestCode == ENABLE_BLUETOOTH) {
            if(resultCode == RESULT_OK) {
                statusMessage.setText("Bluetooth ativado :D");
            }
            else {
                statusMessage.setText("Bluetooth não ativado :(");
            }
        }
        else if(requestCode == SELECT_PAIRED_DEVICE || requestCode == SELECT_DISCOVERED_DEVICE) {
            if (resultCode == RESULT_OK) {
                statusMessage.setText("Você selecionou " + data.getStringExtra("btDevName") + "\n"
                        + data.getStringExtra("btDevAddress"));

                connect = new ConnectionThread(data.getStringExtra("btDevAddress"));
                connect.start();
            } else {
                statusMessage.setText("Nenhum dispositivo selecionado :(");
            }
        }
    }
}
